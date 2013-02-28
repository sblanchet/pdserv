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
 *  vim:tw=78
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
#define pd_double_T     1
#define pd_single_T     2
#define pd_uint8_T      3
#define pd_sint8_T      4
#define pd_uint16_T     5
#define pd_sint16_T     6
#define pd_uint32_T     7
#define pd_sint32_T     8
#define pd_uint64_T     9
#define pd_sint64_T     10
#define pd_boolean_T    11
#define pd_datatype_end 12

/** Structure declarations.
 */
struct pdserv;
struct pdtask;
struct pdvariable;
struct pdevent;

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

/** Set the path to a configuration file
 *
 * This file will only be loaded after calling pdserv_prepare() in
 * non-real-time context.
 *
 * Failure to load will be reported in syslog
 *
 * A configuration file may also be set using the PDSERV_CONFIG environment
 * variable. Failing that, the default configuration
 *      ${etherlab_dir}/etc/pdserv.conf
 * will be attempted
 */
void pdserv_config_file(
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

/** Create compound data type
 *
 * Use this to create an application specific data type.
 * Use @pdserv_compound_add_field to insert fields
 *
 * returns:
 *      handle to compound data type
 */
int pdserv_create_compound(
        const char *name,       /**< Data type name */
        size_t size             /**< Size of compound */
        );

/** Add a field to a compound data type
 */
void pdserv_compound_add_field(
        int compound,           /**< Compound handle */
        const char *name,       /**< Data type name */
        int data_type,          /**< Field data type */
        size_t offset,          /**< Offset of field */
        size_t ndim,            /**< Number of dimensions */
        const size_t *dim       /**< Dimensions */
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
        int datatype,             /**< Signal data type */
        const void *addr,         /**< Signal address */
        size_t n,                 /**< Element count.
                                   * If @dim != NULL, this is the number
                                   * elements in @dim */
        const size_t *dim         /**< Dimensions. If NULL, consider the
                                   * parameter to be a vector of length @n */
        );

/** Register an event channel
 *
 * Event channels are used to implement a messaging system. A clear text
 * message corresponding to the event element is printed when the event is set
 * using pdserv_event_set().
 *
 */
#define EMERG_EVENT     0
#define ALERT_EVENT     1
#define CRIT_EVENT      2
#define ERROR_EVENT     3
#define WARN_EVENT      4
#define NOTICE_EVENT    5
#define INFO_EVENT      6
#define DEBUG_EVENT     7
const struct pdevent *pdserv_event(
        struct pdserv* pdserv,  /**< Pointer to pdserv structure */
        const char *path,       /**< Event path */
        int priority,           /**< 0 = Emergency
                                 *   1 = Alert
                                 *   2 = Critical
                                 *   3 = Error
                                 *   4 = Warning
                                 *   5 = Notice
                                 *   6 = Info
                                 *   7 = Debug */
        size_t n,               /**< Vector size */
        const char *message[]   /**< Event message list.
                                 * @message can be NULL. If not, there must
                                 * be at least n messages. 
                                 *
                                 * PdServ does not make a copy of these
                                 * messages. They must be available
                                 * permanently. */
        );

/** Set the state of a single event.
 *
 * This function is used to set or reset an event. Setting the event causes
 * the corresponding message to be printed to log and all clients.
 */
void pdserv_event_set(
        const struct pdevent *event,
        size_t element,         /**< Event element */
        char state,             /**< boolean value */
        const struct timespec *t /**< Current time */
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
        int datatype,             /**< Parameter data type */
        void *addr,               /**< Parameter address */
        size_t n,                 /**< Element count.  If @dim != NULL, this
                                   * is the number elements in * @dim */
        const size_t *dim,        /**< Dimensions. If NULL, consider the
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
