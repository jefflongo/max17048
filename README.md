# MAX17048

MAX17048 is a lithium-ion fuel guage capable of measuring state of charge. This driver exposes the register access.

This driver can easily be ported to a custom platform. The only requirements are an i2c implementation (add an implementation to the platform-independent wrapper library [here](https://github.com/jefflongo/libi2c))
