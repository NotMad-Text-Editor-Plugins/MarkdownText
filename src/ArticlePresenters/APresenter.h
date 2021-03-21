#pragma once
#ifndef APRESENTER_H
#define APRESENTER_H

# define AUTO_TYPE -1
# define MINILINK_WKE_TYPE 0
# define MINILINK_TYPE 1
# define BROWSERWIDGET_TYPE 2
# define WEBVIEW2_TYPE 3

#include "APresentee.h"

class APresenter
{
public:
	char wke_mb=0;
	int initWebViewImpl(int kernelType, APresentee * presentee, bool fallback);
};

#endif