#pragma once

#include "HornetBaseExport.h"

enum HITEM_TYPE
{
    HI_UNKNOWN,
    HI_NODE,
    HI_ELEMENT,
};

enum HCATEGORY_TYPE
{
    HC_UNKNOWN,
    HC_NODE,
    HC_ELEMENT,
};

enum HDATABASE_STATUS_FLAG
{
    HDB_STATUS_ADD = 0x01,
    HDB_STATUS_DEL = 0x02,
    HDB_STATUS_MOD = 0x04,
};

typedef unsigned long TId;
#define NO_ID 0;