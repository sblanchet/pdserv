#ifdef __cplusplus
extern "C" {
#endif

/** Structure declaration */
struct rtlab;

/** Initialise rtlab library
 *
 * This is the first call that initialises the library. It should be called
 * before any other library calls
 * returns:
 *      NULL on error
 *      pointer to struct rtlab on success
 */
struct rtlab* rtlab_init(
        int argc,               /**< Argument count */
        const char *argv[],     /**< Arguments */
        int nst                 /**< Number of sample times */
        );

/** Cleanup rtlab */
void rtlab_exit(
        struct rtlab*           /**< Pointer to rtlab structure */
        );

#ifdef __cplusplus
}
#endif
