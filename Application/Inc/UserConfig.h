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



#ifndef USERCONFIG_H_
#define USERCONFIG_H_

struct UserConfig;

#ifdef __cplusplus
extern "C" {
#endif


/**
 * The following "C" interface is used by middleware provided by third parties, which is
 * written exclusively in "C".
 */

typedef struct UserConfig * UCHandle;
void GetCloudKey(UCHandle handle, const uint8_t **, const uint16_t ** );
void GetCloudCert(UCHandle handle, const uint8_t **, const uint16_t ** );
void GetCloudEndpointUrl(UCHandle, const char ** );
void GetCloudThingName(UCHandle, const char **);

#ifdef __cplusplus
}
#endif /* extern "C" */

#endif /* USERCONFIG_H_ */
