#pragma once

#define TWILI_RESULT(code) (((code) << 9) | 0xEF)

#define TWILI_ERR_INVALID_NRO TWILI_RESULT(1)
