/*****************************************************************************
 *
 *  $Id$
 *
 *  Copyright 2010 - 2011  Richard Hacker (lerichi at gmx dot net)
 *
 *  This file is part of the pdserv library.
 *
 *  The pdserv library is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Lesser General Public License as published
 *  by the Free Software Foundation, either version 3 of the License, or (at
 *  your option) any later version.
 *
 *  The pdserv library is distributed in the hope that it will be useful, but
 *  WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 *  or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public
 *  License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public License
 *  along with the pdserv library. If not, see <http://www.gnu.org/licenses/>.
 *
 *****************************************************************************/

#ifndef PDSERV_H
#define PDSERV_H

#include <stddef.h>
#include <time.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Data type definitions.
 *
 * Let the enumeration start at 1 so that an unset data type could be
 * detected.
 */
enum pdserv_datatype_t {
    pd_double_T = 1, pd_single_T,
    pd_uint8_T,      pd_sint8_T,
    pd_uint16_T,     pd_sint16_T,
    pd_uint32_T,     pd_sint32_T,
    pd_uint64_T,     pd_sint64_T,
    pd_boolean_T
};

/** Structure declarations.
 */
struct pdserv;
struct pdtask;
struct pdvariable;

/** Initialise pdserv library.
 *
 * This is the first call that initialises the library. It should be called
 * before any other library calls.
 *
 * returns:
 *      NULL on error
 *      pointer to struct pdserv on success
 */
struct pdserv* pdserv_create(
        const char *name,       /**< Name of the process */
        const char *version,    /**< Version string */
        int (*gettime)(struct timespec*)   /**< Function used by the library
                                            * when the system time is required.
                                            * Essentially, this function
                                            * should call clock_gettime().
                                            * If NULL, gettimeofday() is
                                            * used. */
        );

int pdserv_config_file(
        struct pdserv* pdserv, /**< Pointer to pdserv struct */
        const char *file
        );

/** Create a cyclic task
 *
 * An application consists of one or more tasks that are called cyclically by
 * caller.
 *
 * returns:
 *      NULL on error
 *      pointer to struct pdserv on success
 */
struct pdtask* pdserv_create_task(
        struct pdserv* pdserv,  /**< Pointer to pdserv struct */
        double tsample,         /**< Base sample time of task */
        const char *name        /**< Optional name string */
        );

/** Register a signal
 *
 * This call registers a signal, i.e. Variables that are calculated
 *
 * Arguments @n and @dim are used to specify the shape:
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
struct pdvariable *pdserv_signal(
        struct pdtask* pdtask,    /**< Pointer to pdtask structure */
        unsigned int decimation,  /**< Decimation with which the signal is
                                   * calculated */
        const char *path,         /**< Signal path */
        enum pdserv_datatype_t datatype, /**< Signal data type */
        const void *addr,         /**< Signal address */
        size_t n,                 /**< Element count.
                                   * If @dim != NULL, this is the number
                                   * elements in @dim */
        const size_t dim[]        /**< Dimensions. If NULL, consider the
                                   * parameter to be a vector of length @n */
        );

/** Callback on a parameter update event
 *
 * See \pdserv_set_parameter_trigger
 *
 * This function is responsible for copying the data from @src to @dst
 *
 * \returns 0 on success
 */
typedef int (*paramtrigger_t)(
        struct pdtask *pdtask,  /**< Pointer to pdtask structure */
        const struct pdvariable *param, /**< Pointer to parameter */
        void *dst,              /**< Destination address @addr */
        const void *src,        /**< Data source */
        size_t len,             /**< Data length in bytes */
        void *priv_data         /**< Optional user variable */
        );

/** Register a parameter
 *
 * This call registers a parameter. See @pdserv_signal for the description of
 * similar function arguments.
 *
 * During the registration of a parameter, the caller has the chance to
 * register callback functions which are called inside (@trigger) and outside
 * (@paramcheck) of real time context when a parameter is updated.  The
 * function is responsible for copying, and optionally modifying, data from
 * @src to @dst. The value of @priv_data will be passed to the callback
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
struct pdvariable *pdserv_parameter(
        struct pdserv* pdserv,    /**< Pointer to pdserv structure */
        const char *path,         /**< Parameter path */
        unsigned int mode,        /**< Access mode, same as Unix file mode */
        enum pdserv_datatype_t datatype, /**< Parameter data type */
        void *addr,               /**< Parameter address */
        size_t n,                 /**< Element count.  If @dim != NULL, this
                                   * is the number elements in * @dim */
        const size_t dim[],       /**< Dimensions. If NULL, consider the
                                   * parameter to be a vector of length @n */
        paramtrigger_t trigger,   /**< Callback for updating the parameter
                                   * inside real time context */
        void *priv_data           /**< Arbitrary pointer passed to @trigger
                                   * when it is called */
        );

/** Set an optional alias for a variable
 *
 * Sometimes it it easier to find a variable by its alias. Use this
 * function after calling either @pdserv_signal or @pdserv_parameter
 * to set the alias name for a variable.
 */
void pdserv_set_alias(
        struct pdvariable *variable, /**< Parameter or Signal address */
        const char *alias       /**< Variable's alias */
        );

/** Set the optional unit of a variable */
void pdserv_set_unit(
        struct pdvariable *variable, /**< Parameter or Signal address */
        const char *unit        /**< Variable's unit */
        );

/** Set the optional comment of a variable */
void pdserv_set_comment(
        struct pdvariable *variable, /**< Parameter or Signal address */
        const char *comment     /**< Variable's comment */
        );

/** Finish initialisation
 *
 * This function is called after registering all variables and parameters,
 * just before going into cyclic real time. This will fork off the process
 * that communicates with the rest of the world.
 *
 * Since the new process lives in a separate memory space, all parameters must
 * have been initialised beforehand. After calling @pdserv_prepare, parameters
 * should not be changed any more. They can only be updated though a call by
 * the library.
 */
int pdserv_prepare(
        struct pdserv* pdserv     /**< Pointer to pdserv structure */
        );

/** Update parameters
 *
 * Call this function to get parameters
 */
void pdserv_get_parameters(
        struct pdserv* pdserv,
        struct pdtask* pdtask,  /**< Pointer to pdtask structure */
        const struct timespec *t
        );

/** Update task statistics variables
 *
 * Call this function at the end of the calculation cycle just before
 * @pdserv_update to update statistics
 */
void pdserv_update_statistics(
        struct pdtask* pdtask,  /**< Pointer to pdtask structure */
        double exec_time,       /**< Execution time in [s] */
        double cycle_time,      /**< Time since last call */
        unsigned int overrun    /**< total overrun count */
        );

/** Update variables
 *
 * Call this function at the end of the calculation cycle to update
 * variables
 */
void pdserv_update(
        struct pdtask* pdtask,  /**< Pointer to pdtask structure */
        const struct timespec *t  /**< Current model time.
                                  * If NULL, zero is assumed */
        );

/** Cleanup pdserv */
void pdserv_exit(
        struct pdserv*           /**< Pointer to pdserv structure */
        );

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* PDSERV_H */
