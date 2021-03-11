void print_err(const char *msg, ...);
void print_errcode(int err, const char *msg, ...);

int mac_to_unix(unsigned char **outptr, unsigned char *outend,
                const unsigned char **inptr, const unsigned char *inend);

int mac_from_unix(unsigned char **outptr, unsigned char *outend,
                  const unsigned char **inptr, const unsigned char *inend);

int mac_from_unix_init(void);
void mac_from_unix_term(void);
