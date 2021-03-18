# MAX17048

MAX17048 is a lithium-ion fuel guage capable of measuring state of charge. This driver exposes the register access.

This driver can easily be ported to a custom platform. The only requirements are an i2c implementation. Add an implementation to the [platform-independent i2c wrapper library](https://github.com/jefflongo/libi2c). Please check the wrapper library README for details about providing the implementation. `i2c_master_init` should be called before using any of the max17048 driver functions. If there are additional requirements for porting the code to your own platform, please submit an issue so that the compatability can be further improved in the future.

To test if the i2c implementation is successful,`max17048_is_present()` should return true with the BQ24292i connected to the i2c bus.
