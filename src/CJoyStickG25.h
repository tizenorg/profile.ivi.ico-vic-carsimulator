#ifndef CJOYSTICKG25_H_
#define CJOYSTICKG25_H_
#include "CJoyStickEV.h"

class CJoyStickG25 : public CJoyStickEV
{
public:
    CJoyStickG25();
    ~CJoyStickG25();

    virtual int Open();
};

#endif /* CJOYSTICKG25_H_ */
