#pragma once

#include "headers.h"

#define console_is_open (console.y_spring.current > 0.1f)

char* find_command_suggestion(void);

char* find_current_command(void);
