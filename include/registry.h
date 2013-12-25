/******************************************************************************

            Copyright (c) 1996 - 2004  --  University of Washington

******************************************************************************/

#ifndef _REGISTRY_H
#define _REGISTRY_H

#define _REGISTER _register
#define _UNREGISTER _unregister

#define _REGISTRY_PTR(reg) ((reg)->ptr)
#define _REGISTRY_NEXT(reg) ((reg)->next)

typedef struct __registry _registry;

struct __registry {
  void* ptr;
  _registry* next;
};

_registry* _register(_registry*, void*);
_registry* _unregister(_registry*, void*);

#endif
