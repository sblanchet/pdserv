#ifndef PDCOMSERV_H
#define PDCOMSERV_H

#include "etl_data_info.h"
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Structure declaration */
struct pdcomserv;
struct variable;

/** Initialise pdcomserv library
 *
 * This is the first call that initialises the library. It should be called
 * before any other library calls
 * returns:
 *      NULL on error
 *      pointer to struct pdcomserv on success
 */
struct pdcomserv* pdcomserv_create(
        int argc,               /**< Argument count */
        const char *argv[],     /**< Arguments */
        const char *name,       /**< Name of the process */
        const char *version,    /**< Version string */
        double baserate,        /**< Sample time of tid0 */
        unsigned int nst,       /**< Number of sample times */
        const unsigned int decimation[],   /**< Array of the decimations
                                            * for tid1..tid<nst> */
        int (*gettime)(struct timespec*)   /**< Function used by the library
                                            * when the system time is required.
                                            * Essentially, this function
                                            * should call clock_gettime().
                                            * If NULL, gettimeofday() is
                                            * used */
        );

/** Register a signal
 *
 * This call registers a signal, i.e. Variables that are calculated
 *
 * Parameters @n and @dim are used to specify the shape:
 * For scalars:
 *    @n = 1
 *    @dim = NULL
 *
 * For vectors length x
 *    @n = x
 *    @dim = NULL
 *
 * For arrays
 *   int ary[2][3][4]
 *
 *   @n = 3;
 *   @dim = {2,3,4}
 *   
 */
struct variable* pdcomserv_signal(
        struct pdcomserv* pdcomserv,    /**< Pointer to pdcomserv structure */
        unsigned int tid,         /**< Task Id of the signal */
        unsigned int decimation,  /**< Decimation with which the signal is
                                   * calculated */
        const char *path,         /**< Signal path */
        enum si_datatype_t datatype, /**< Signal data type */
        const void *addr,         /**< Signal address */
        size_t n,                 /**< Element count.
                                   * If @dim != NULL, this is the number
                                   * elements in @dim */
        const size_t dim[]        /**< Dimensions. If NULL, consider the
                                   * parameter to be a vector of length @n */
        );

/** Callback on a parameter update event
 *
 * See \pdcomserv_set_parameter_trigger
 *
 * This function is responsible for copying the data from @src to @dst
 *
 * \returns 0 on success
 */
typedef int (*paramtrigger_t)(
        unsigned int tid,       /**< Task id context of call */
        char checkOnly,         /**< Only check the data, the copy comes
                                 * later */
        void *dst,              /**< Destination address @addr */
        const void *src,        /**< Data source */
        size_t len,             /**< Data length in bytes */
        void *priv_data         /**< Optional user varialbe */
        );

/** Register a parameter
 *
 * This call registers a parameter. See @pdcomserv_signal for the description of
 * similar function arguments.
 *
 * During the registration of a parameter, the caller has the chance to
 * register callback functions which are called inside (@paramupdate) and
 * outside (@paramcheck) of real time context when a parameter is updated.  The
 * function is responsible for copying, and optionally modifying, data from
 * @src to @dst.  The value of @priv_data will be passed to the callback
 * function.
 *
 * By default @memcpy will be used automatically for an unspecified callback.
 *
 * Outside real time context, the function can for example check the incoming
 * values.  Since it is not in real time context, timing is irrelevant. If the
 * function returns a ZERO, the library will transmit the data in @dst to real
 * time context. Non-zero return values will cause a failed parameter write to
 * be reported. Typically, negative error numbers should be returned.
 *
 * Inside real time context, the function not only has the chance to modify the
 * data while copying from @src to @dst, it also has the chance to trigger
 * events at the same time for example. Incidentally @dst is the address
 * specified by @addr during registration. The return value is ignored.
 */
struct variable *pdcomserv_parameter(
        struct pdcomserv* pdcomserv,    /**< Pointer to pdcomserv structure */
        const char *path,         /**< Parameter path */
        unsigned int mode,        /**< Access mode, same as unix file mode */
        enum si_datatype_t datatype, /**< Parameter data type */
        void *addr,               /**< Parameter address */
        size_t n,                 /**< Element count.  If @dim != NULL, this
                                   * is the number elements in * @dim */
        const size_t dim[],       /**< Dimensions. If NULL, consider the
                                   * parameter to be a vector of length @n */
        paramtrigger_t paramopy,  /**< Callback for updating the parameter
                                   * inside real time context */
        void *priv_data           /**< Arbitrary pointer passed to @paramcopy
                                   * when it is called */
        );

/** Set an optional alias for a variable
 *
 * Sometimes it it easier to find a variable by its alias. Use this
 * function after calling either @pdcomserv_signal or @pdcomserv_parameter
 * to set the alias name for a variable.
 */
void pdcomserv_set_alias(
        struct variable *variable, /**< Parameter or Signal address */
        const char *alias       /**< Variable's alias */
        );

/** Set the optional unit of a variable */
void pdcomserv_set_unit(
        struct variable *variable, /**< Parameter or Signal address */
        const char *unit        /**< Variable's unit */
        );

/** Set the optional comment of a variable */
void pdcomserv_set_comment(
        struct variable *variable, /**< Parameter or Signal address */
        const char *comment     /**< Variable's comment */
        );


/** Finish initialisation
 *
 * This function is called after registering all variables and parameters,
 * just before going into cyclic real time. This will fork off the process
 * that communicates with the rest of the world.
 *
 * Since the new process lives in a separate memory space, all parameters must
 * have been initialised beforehand. After calling @pdcomserv_init, parameters
 * should not be changed any more. They can only be updated though a call by
 * the library.
 */
int pdcomserv_init(
        struct pdcomserv* pdcomserv     /**< Pointer to pdcomserv structure */
        );

/** Update variables
 *
 * Call this function at the end of the calculation cycle to update
 * variables
 */
void pdcomserv_update(
        struct pdcomserv* pdcomserv,  /**< Pointer to pdcomserv structure */
        int st,                 /**< Sample task id to update */
        const struct timespec *t /**< Current model time.
                                  * If NULL, zero is assumed */
        );

/** Cleanup pdcomserv */
void pdcomserv_exit(
        struct pdcomserv*           /**< Pointer to pdcomserv structure */
        );

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // PDCOMSERV_H