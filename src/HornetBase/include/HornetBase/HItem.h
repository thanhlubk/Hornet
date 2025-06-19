#pragma once

#include "HornetBaseExport.h"
#include "HDataBaseDef.h"

#pragma pack(push, 4)
class HORNETBASE_EXPORT HItem
{
public:
    HItem(TId id)
    {
        m_Id = id;
    }

    virtual ~HItem() {}
private:
    TId m_Id;
};
