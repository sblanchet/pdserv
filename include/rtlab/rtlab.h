#include <rtlab/etl_data_info.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Structure declaration */
struct hrtlab;

/** Initialise hrtlab library
 *
 * This is the first call that initialises the library. It should be called
 * before any other library calls
 * returns:
 *      NULL on error
 *      pointer to struct hrtlab on success
 */
struct hrtlab* hrtlab_init(
        int argc,               /**< Argument count */
        const char *argv[],     /**< Arguments */
        int nst                 /**< Number of sample times */
        );

/** Register a signal
 *
 * This call registers a signal, i.e. Variables that are calculated
 */
int hrtlab_signal(
        struct hrtlab* hrtlab,    /**< Pointer to hrtlab structure */
        unsigned int tid,         /**< Task Id of the signal */
        unsigned int decimation,  /**< Decimation with which the signal is
                                   * calculated */
        const char *path,         /**< Signal path */
        const char *alias,        /**< Signal alias */
        enum si_datatype_t datatype, /**< Signal data type */
        unsigned int ndims,       /**< Number of dimensions */
        const unsigned int dim[], /**< Dimensions */
        const void *addr          /**< Signal address */
        );

typedef int (*paramcheck_t)();

/** Register a signal
 *
 * This call registers a signal, i.e. Variables that are calculated
 */
int hrtlab_parameter(
        struct hrtlab* hrtlab,    /**< Pointer to hrtlab structure */
        paramcheck_t paramcheck,
        void *priv_data,
        const char *path,         /**< Signal path */
        const char *alias,        /**< Signal alias */
        enum si_datatype_t datatype, /**< Signal data type */
        unsigned int ndims,       /**< Number of dimensions */
        const unsigned int dim[], /**< Dimensions */
        const void *addr          /**< Signal address */
        );

/** Finish initialisation
 *
 * This function is called after registering all variables and parameters,
 * just before going into cyclic real time
 */
int hrtlab_start(
        struct hrtlab* hrtlab     /**< Pointer to hrtlab structure */
        );

/** Update variables
 *
 * Call this function at the end of the calculation cycle to update
 * variables
 */
void hrtlab_update(
        struct hrtlab* hrtlab,    /**< Pointer to hrtlab structure */
        int st                  /**< Sample task id to update */
        );

/** Cleanup hrtlab */
void hrtlab_exit(
        struct hrtlab*           /**< Pointer to hrtlab structure */
        );

#ifdef __cplusplus
}
#endif
