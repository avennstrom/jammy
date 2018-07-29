#pragma once

#define jm_max(a, b) (((a) > (b)) ? (a) : (b))
#define jm_min(a, b) (((a) < (b)) ? (a) : (b))
#define jm_clamp(val, lower, upper) (jm_max(lower, jm_min(upper, val)))