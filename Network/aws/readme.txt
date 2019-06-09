

hdr: \demos\common\include  - credential keys, exclude unknown files (polute namespace)
hdr: \demos\st\stm32l475_discovery\common\config_files  - exclude FreeRTOS headers, include various config files, platform specific
mix: \demos\st\stm32l475_discovery\common\application_code\st_code  - exclude HAL, include others


mix: \lib\third_party\mcu_vendor\st\stm32l475_discovery\BSP\Components\es_wifi  - exclude template header file
hdr: \lib\include  - exclude FreeRTOS headers...


hdr: \lib\include\private - all...exclude unknown files (polute namespace)
hdr: \lib\third_party\pkcs11 - all
src: \lib\mqtt  - all
src: \lib\pkcs11\mbedtls  - all
src: \lib\pkcs11\portable\st\stm32l475_discovery - all
src: \lib\secure_sockets\portable\st\stm32l475_discovery - all
src: \lib\tls - all
src: \lib\utils - all

mix: \lib\third_party\mbedtls - obtain from source




AWS Core (linked):
hdr: \lib\include  - exclude FreeRTOS headers...
hdr: \lib\include\private - all...exclude unknown files (polute namespace)
hdr: \lib\third_party\pkcs11 - all
src: \lib\mqtt  - all
src: \lib\pkcs11\mbedtls  - all
src: \lib\tls - all
src: \lib\utils - all


AWS Project specfic (not linked):
hdr: \demos\common\include  - credential keys, exclude unknown files (polute namespace)
hdr: \demos\st\stm32l475_discovery\common\config_files  - exclude FreeRTOS headers, include various config files, platform specific
mix: \demos\st\stm32l475_discovery\common\application_code\st_code  - exclude HAL, include others
mix: \lib\third_party\mcu_vendor\st\stm32l475_discovery\BSP\Components\es_wifi  - exclude template header file
src: \lib\pkcs11\portable\st\stm32l475_discovery - all
src: \lib\secure_sockets\portable\st\stm32l475_discovery - all


Other:
mix: \lib\third_party\mbedtls - obtain from source
