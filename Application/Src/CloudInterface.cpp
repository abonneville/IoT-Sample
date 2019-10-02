/*
 * Copyright (C) 2019 Andrew Bonneville.  All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 */

#include <cstring>

#include "ThreadConfig.hpp"
#include "UserConfig.hpp"
#include "CloudInterface.hpp"

#include "WiFiStation.hpp"
#include "hts221.hpp"
#include "lps22hb.hpp"

#include "UserConfig.h"

extern "C" {
#include "aws_mqtt_agent.h"
}

/* Typedef -----------------------------------------------------------*/

/* Define ------------------------------------------------------------*/
/**
 * @brief MQTT client ID.
 *
 * It must be unique per MQTT broker.
 */
#define MQTT_CLIENT_ID            ( ( const uint8_t * ) "MQTTEcho" )

/* Timeout used when establishing a connection, which required TLS
 * negotiation. */
#define MQTT_TLS_NEGOTIATION_TIMEOUT          pdMS_TO_TICKS( 12000 )

/* Timeout used when performing MQTT operations that do not need extra time
 * to perform a TLS negotiation. */
#define MQTT_TIMEOUT                               pdMS_TO_TICKS( 2500 )

/* Port number the MQTT broker is using. */
#define MQTT_BROKER_PORT             8883

/* Topic name for the MQTT broker */
#define TOPIC_NAME (const uint8_t *)"stm32/sensor"


/* Macro -------------------------------------------------------------*/

/* Variables ---------------------------------------------------------*/
enl::WiFiStation WiFi;

static MQTTAgentHandle_t xMQTTHandle = NULL;
static const size_t buf_size = 512;
static char buf[buf_size] = {};


/* Function prototypes -----------------------------------------------*/
static bool networkInit(UserConfig &userConfig);
static bool cloudConnect(UserConfig &userConfig);
static void cloudDisconnect();
static void cloudSend(int16_t temperature, uint16_t humidity, uint16_t pressure);


/* External functions ------------------------------------------------*/


/**
 * @brief Creates the thread object, if the scheduler is already running, then then thread
 * will begin.
 */
CloudInterface::CloudInterface(UserConfig &uh)
	: Thread("CloudInterface", STACK_SIZE_CLOUD, THREAD_PRIORITY_NORMAL),
	  userConfigHandle(uh)
{
	Start();
}



/**
 * @brief Implements the persistent loop for thread execution.
 */
void CloudInterface::Run()
{
	uint16_t humidity, pressure;
	int16_t temperature;

	sensor::HTS221 hts221(I2C2_Bus);
	sensor::LPS22HB lps22hb(I2C2_Bus);

	networkInit(userConfigHandle);

	if( cloudConnect(userConfigHandle) )
	{
		for(size_t index = 0; index < 10; index++)
		{
			temperature = hts221.getTemperature();
			humidity = hts221.getHumidity();
			pressure = lps22hb.getPressure();

			cloudSend(temperature, humidity, pressure);

			CloudInterface::DelayUntil(5000);
		}

		cloudDisconnect();
		WiFi.disconnect();
	}

    configPRINTF( ("Demo completed.\n") );

	/* Suspend ourselves indefinitely */
	vTaskSuspend(NULL);
	while (1);
}

/**
 * @brief Establishes a valid network connection
 * @param userConfig contains the network parameters necessary to establish a network connection
 */
static bool networkInit(UserConfig &userConfig)
{
	/* Connect to the Internet via WiFi */
	const UserConfig::Wifi_t &wifiConfig = userConfig.GetWifiConfig();

	enl::WiFiStatus status = WiFi.begin(wifiConfig.ssid.value.data(),
									    wifiConfig.password.value.data(),
										enl::WiFiSecurityType::Auto);

	if ( status == enl::WiFiStatus::WL_CONNECTED )
	{
	    configPRINTF( ("WiFi connected.\n") );
	}
	else
	{
	    configPRINTF( ("Error: failed to establish WiFi connection with error code %i\n", status) );
	}

	return ( status == enl::WiFiStatus::WL_CONNECTED );
}


/**
 * @brief Establishes a valid MQTT client connection with a cloud server
 * @param userConfig contains the cloud parameters necessary to establish a server connection
 */
static bool cloudConnect(UserConfig &userConfig)
{
    MQTTAgentReturnCode_t xReturned;
    BaseType_t xReturn = pdFAIL;

    /** Grab a copy of the cloud configuration value(s)
     */
    UCHandle handle = &userConfig;

    const char * mqttBrokerEndpointurl = NULL;
    GetCloudEndpointUrl(handle, &mqttBrokerEndpointurl);

    MQTTAgentConnectParams_t xConnectParameters =
    {
    	mqttBrokerEndpointurl, 				  /* The URL of the MQTT broker to connect to. */
		mqttagentREQUIRE_TLS,                 /* Connection flags. */
        pdFALSE,                              /* Deprecated. */
        MQTT_BROKER_PORT,                     /* Port number on which the MQTT broker is listening. Can be overridden by ALPN connection flag. */
        MQTT_CLIENT_ID,                       /* Client Identifier of the MQTT client. It should be unique per broker. */
        0,                                    /* The length of the client Id, filled in later as not const. */
        pdFALSE,                              /* Deprecated. */
        NULL,                                 /* User data supplied to the callback. Can be NULL. */
        NULL,                                 /* Callback used to report various events. Can be NULL. */
        NULL,                                 /* Certificate used for secure connection. Can be NULL. */
        0                                     /* Size of certificate used for secure connection. */
    };

    /* Check this function has not already been executed. */
    configASSERT( xMQTTHandle == NULL );

    /* The MQTT client object must be created before it can be used.  The
     * maximum number of MQTT client objects that can exist simultaneously
     * is set by mqttconfigMAX_BROKERS. */
    xReturned = MQTT_AGENT_Create( &xMQTTHandle );

    if( xReturned == eMQTTAgentSuccess )
    {
        /* Fill in the MQTTAgentConnectParams_t member that is not const,
         * and therefore could not be set in the initializer (where
         * xConnectParameters is declared in this function). */
        xConnectParameters.usClientIdLength = ( uint16_t ) strlen( ( const char * ) MQTT_CLIENT_ID );

        /* Connect to the broker. */
        configPRINTF( ( "MQTT client attempting to connect to %s.\n", mqttBrokerEndpointurl ) );
        xReturned = MQTT_AGENT_Connect( xMQTTHandle,
                                        &xConnectParameters,
                                        MQTT_TLS_NEGOTIATION_TIMEOUT );

        if( xReturned != eMQTTAgentSuccess )
        {
            /* Could not connect, so delete the MQTT client. */
            ( void ) MQTT_AGENT_Delete( xMQTTHandle );
            configPRINTF( ( "ERROR:  MQTT client failed to connect with error %d.\n", xReturned ) );
        }
        else
        {
            configPRINTF( ( "MQTT client connected.\n" ) );
            xReturn = pdPASS;
        }
    }

	return ( xReturn == pdPASS );
}


/**
 * @brief Closes an active MQTT connection.
 */
static void cloudDisconnect()
{
	MQTT_AGENT_Disconnect( xMQTTHandle, MQTT_TIMEOUT );
    MQTT_AGENT_Delete( xMQTTHandle );
    configPRINTF( ( "MQTT client disconnected.\n" ) );
}


/**
 * @brief Uploads the provided sensor information to the cloud server
 * @param temperature value to be uploaded
 * @param humidity value to be uploaded
 * @param pressure value to be uploaded
 */
static void cloudSend(int16_t temperature, uint16_t humidity, uint16_t pressure)
{
    MQTTAgentPublishParams_t xPublishParameters;

    /* Format sensor values into a JSON message */
    int32_t ret = snprintf(buf, buf_size,
                "{\"sensor\":{\"temperature\":%i,\"humidity\":%.u,\"pressure\":%u}}",
                temperature, humidity, pressure);
    if(ret < 0) {
    	configPRINTF( ("ERROR: snprintf() returns %lu.", ret) );
    }


    /* Setup the publish parameters. */
    memset( &( xPublishParameters ), 0x00, sizeof( xPublishParameters ) );
    xPublishParameters.pucTopic = TOPIC_NAME;
    xPublishParameters.pvData = buf;
    xPublishParameters.usTopicLength = ( uint16_t ) strlen( ( const char * ) TOPIC_NAME );
    xPublishParameters.ulDataLength = ret;
    xPublishParameters.xQoS = eMQTTQoS1;

    /* Publish the message. */
    MQTTAgentReturnCode_t xReturned = MQTT_AGENT_Publish( xMQTTHandle,
                                    &( xPublishParameters ),
                                    MQTT_TIMEOUT );

    if( xReturned != eMQTTAgentSuccess )
    {
    	configPRINTF( ("ERROR: xReturned from MQTT publish is %d\n", xReturned) );
    }

    configPRINTF( ("Message published '%s'\n", buf ) );
}


