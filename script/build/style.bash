capital letters only used for variables declared const

all #defines prefixed by d_

systems (ode, opl, etc.) communicate in a single file prefixed by "binder_", outside of this they never include each other

functions that take a struct* as first argument are always prefixed by (that argument type)_ (method style)
constructors prefixed by ctor_
destructors prefixed by dtor_
