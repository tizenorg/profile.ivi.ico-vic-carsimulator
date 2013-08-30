#ifndef CJOYSTICKG27_H_
#define CJOYSTICKG27_H_
#include "CJoyStickEV.h"

class CJoyStickG27 : public CJoyStickEV
{
public:
    CJoyStickG27();
    ~CJoyStickG27();

    virtual int Open();
};

#endif /* CJOYSTICKG27_H_ */
