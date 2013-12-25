/******************************************************************************

            Copyright (c) 1996 - 2004  --  University of Washington

******************************************************************************/

#include <stdio.h>
#include "registry.h"
#include "zlib.h"
#include "zerror.h"

_registry* _register(_registry* reg, void* ptr)
{
  _registry* new = _zmalloc(sizeof(_registry), "registeree");

  _REGISTRY_PTR(new) = ptr;
  _REGISTRY_NEXT(new) = reg;
  return new;
}

_registry* _unregister(_registry* reg, void* ptr)
{
  _registry* tmp;
  _registry* prev;
  _registry* first;

  first = NULL;
  prev = NULL;
  tmp = reg;
  while (tmp) {
    if (ptr == _REGISTRY_PTR(tmp)) {
      break;
    }
    prev = tmp;
    if (!first) {
      first = prev;
    }
    tmp = _REGISTRY_NEXT(tmp);
  }
  if (!tmp) {
    _RT_ANY_FATAL0("Attempt to unregister non-registered entity");
  }
  if (prev) {
    _REGISTRY_NEXT(prev) = _REGISTRY_NEXT(tmp);
  }
  else {
    first = _REGISTRY_NEXT(tmp);
  }
  _zfree(tmp, "_registeree");
  return first;
}
