#A lightweight SSTV encode/decode library written for embedded systems.

The current version of csstv supports the following PD modes for both
encoding and decoding:

PD50

PD90

PD120

PD160

PD180

PD240

PD290

Encoding accepts GRAY8, RGB888, BGR888, and RGB565 input. Decoding writes
caller-owned RGB888 output buffers. 

Additional mode families may be added in future releases.
