#ifndef CLI_H__
#define CLI_H__

#define CLI_MAX_LEN 64
extern char cli_buf[CLI_MAX_LEN + 1]; // +1 to allow for terminating \0

// Wait until a CLI command has been entered into cli_buf and return.
void cli_get(void);

#endif // CLI_H__
