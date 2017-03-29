#include <errno.h>
#include <error.h>
#include <glib.h>
#include <inttypes.h>
#include <pthread.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "connection-manager.h"

static gpointer connection_manager_parent_class = NULL;

enum {
    SIGNAL_0,
    SIGNAL_NEW_CONNECTION,
    N_SIGNALS,
};

enum {
    PROP_0,
    PROP_MAX_CONNECTIONS,
    N_PROPERTIES,
};
static GParamSpec *obj_properties [N_PROPERTIES] = { NULL, };

static guint signals [N_SIGNALS] = { 0, };

/*
 * GObject property setter.
 */
static void
connection_manager_set_property (GObject        *object,
                                 guint           property_id,
                                 GValue const   *value,
                                 GParamSpec     *pspec)
{
    ConnectionManager *mgr = CONNECTION_MANAGER (object);

    g_debug ("connection_manager_set_property: 0x%" PRIxPTR,
             (uintptr_t)mgr);
    switch (property_id) {
    case PROP_MAX_CONNECTIONS:
        mgr->max_connections = g_value_get_uint (value);
        g_debug ("  max_connections: 0x%" PRIxPTR,
                 (uintptr_t)mgr->max_connections);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
        break;
    }
}
/*
 * GObject property getter.
 */
static void
connection_manager_get_property (GObject     *object,
                                 guint        property_id,
                                 GValue      *value,
                                 GParamSpec  *pspec)
{
    ConnectionManager *mgr = CONNECTION_MANAGER (object);

    g_debug ("connection_manager_get_property: 0x%" PRIxPTR, (uintptr_t)mgr);
    switch (property_id) {
    case PROP_MAX_CONNECTIONS:
        g_value_set_uint (value, mgr->max_connections);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
        break;
    }
}

ConnectionManager*
connection_manager_new (guint max_connections)
{
    ConnectionManager *mgr;

    mgr = CONNECTION_MANAGER (g_object_new (TYPE_CONNECTION_MANAGER,
                                            "max-connections", max_connections,
                                            NULL));
    if (pthread_mutex_init (&mgr->mutex, NULL) != 0)
        g_error ("Failed to initialize connection _manager mutex: %s",
                 strerror (errno));
    /* These two data structures must be kept in sync. When the
     * connection-manager object is destoryed the Connection objects in these
     * hash tables will be free'd by the g_object_unref function. We only
     * set this for one of the hash tables because we only want to free
     * each Connection object once.
     */
    mgr->connection_from_fd_table =
        g_hash_table_new_full (g_int_hash,
                               connection_equal_fd,
                               NULL,
                               NULL);
    mgr->connection_from_id_table =
        g_hash_table_new_full (g_int64_hash,
                               connection_equal_id,
                               NULL,
                               (GDestroyNotify)g_object_unref);
    return mgr;
}

static void
connection_manager_finalize (GObject *obj)
{
    gint ret;
    ConnectionManager *manager = CONNECTION_MANAGER (obj);

    ret = pthread_mutex_lock (&manager->mutex);
    if (ret != 0)
        g_error ("Error locking connection_manager mutex: %s",
                 strerror (errno));
    g_hash_table_unref (manager->connection_from_fd_table);
    g_hash_table_unref (manager->connection_from_id_table);
    ret = pthread_mutex_unlock (&manager->mutex);
    if (ret != 0)
        g_error ("Error unlocking connection_manager mutex: %s",
                 strerror (errno));
    ret = pthread_mutex_destroy (&manager->mutex);
    if (ret != 0)
        g_error ("Error destroying connection_manager mutex: %s",
                 strerror (errno));
}
/**
 * Boilerplate GObject class init function. The only interesting thing that
 * we do here is creating / registering the 'new_connection' signal. This
 * signal invokes callbacks with the new_connection_callback type (see
 * header). This signal is emitted by the connection_manager_insert function
 * which is where we add new Connection objects to those tracked by the
 * ConnectionManager.
 */
static void
connection_manager_class_init (gpointer klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS (klass);

    if (connection_manager_parent_class == NULL)
        connection_manager_parent_class = g_type_class_peek_parent (klass);
    object_class->finalize = connection_manager_finalize;
    object_class->get_property = connection_manager_get_property;
    object_class->set_property = connection_manager_set_property;
    signals [SIGNAL_NEW_CONNECTION] =
        g_signal_new ("new-connection",
                      G_TYPE_FROM_CLASS (object_class),
                      G_SIGNAL_RUN_LAST | G_SIGNAL_NO_RECURSE | G_SIGNAL_NO_HOOKS,
                      0,
                      NULL,
                      NULL,
                      NULL,
                      G_TYPE_INT,
                      1,
                      TYPE_CONNECTION);
    obj_properties [PROP_MAX_CONNECTIONS] =
        g_param_spec_uint ("max-connections",
                           "max connections",
                           "Maximum nunmber of concurrent client connections",
                           0,
                           MAX_CONNECTIONS,
                           MAX_CONNECTIONS_DEFAULT,
                           G_PARAM_READWRITE);
    g_object_class_install_properties (object_class,
                                       N_PROPERTIES,
                                       obj_properties);
}

GType
connection_manager_get_type (void)
{
    static GType type = 0;

    if (type == 0) {
        type = g_type_register_static_simple (G_TYPE_OBJECT,
                                              "ConnectionManager",
                                              sizeof (ConnectionManagerClass),
                                              (GClassInitFunc) connection_manager_class_init,
                                              sizeof (ConnectionManager),
                                              NULL,
                                              0);
    }
    return type;
}
gint
connection_manager_insert (ConnectionManager    *manager,
                           Connection        *connection)
{
    gint ret;

    ret = pthread_mutex_lock (&manager->mutex);
    if (ret != 0)
        g_error ("Error locking connection_manager mutex: %s",
                 strerror (errno));
    if (connection_manager_is_full (manager)) {
        g_warning ("connection_manager: 0x%" PRIxPTR " max_connections of %u exceeded",
                   (uintptr_t)manager, manager->max_connections);
        pthread_mutex_unlock (&manager->mutex);
        return -1;
    }
    /*
     * Increase reference count on Connection object on insert. The
     * corresponding call to g_hash_table_remove will cause the reference
     * count to be decreased (see g_hash_table_new_full).
     */
    g_object_ref (connection);
    g_hash_table_insert (manager->connection_from_fd_table,
                         connection_key_fd (connection),
                         connection);
    g_hash_table_insert (manager->connection_from_id_table,
                         connection_key_id (connection),
                         connection);
    ret = pthread_mutex_unlock (&manager->mutex);
    if (ret != 0)
        g_error ("Error unlocking connection_manager mutex: %s",
                 strerror (errno));
    /* not sure what to do about reference count on SEssionData obj */
    g_signal_emit (manager,
                   signals [SIGNAL_NEW_CONNECTION],
                   0,
                   connection,
                   &ret);
    return ret;
}
/*
 * Lookup a Connection object from the provided connection fd. This function
 * returns a reference to the Connection object. The reference count for
 * this object is incremented before it is returned and must be decremented
 * by the caller.
 */
Connection*
connection_manager_lookup_fd (ConnectionManager *manager,
                              gint               fd_in)
{
    Connection *connection;

    pthread_mutex_lock (&manager->mutex);
    connection = g_hash_table_lookup (manager->connection_from_fd_table,
                                      &fd_in);
    pthread_mutex_unlock (&manager->mutex);
    g_object_ref (connection);

    return connection;
}
/*
 * Lookup a Connection object from the provided Connection ID. This function
 * returns a reference to the Connection object. The reference count for
 * this object is incremented before it is returned and must be decremented
 * by the caller.
 */
Connection*
connection_manager_lookup_id (ConnectionManager   *manager,
                              gint64               id)
{
    Connection *connection;

    g_debug ("locking manager mutex");
    pthread_mutex_lock (&manager->mutex);
    g_debug ("g_hash_table_lookup: connection_from_id_table");
    connection = g_hash_table_lookup (manager->connection_from_id_table,
                                      &id);
    g_debug ("unlocking manager mutex");
    pthread_mutex_unlock (&manager->mutex);
    g_object_ref (connection);

    return connection;
}

gboolean
connection_manager_remove (ConnectionManager   *manager,
                           Connection          *connection)
{
    gboolean ret;

    g_debug ("connection_manager 0x%" PRIxPTR " removing Connection 0x%" PRIxPTR,
             (uintptr_t)manager, (uintptr_t)connection);
    pthread_mutex_lock (&manager->mutex);
    ret = g_hash_table_remove (manager->connection_from_fd_table,
                               connection_key_fd (connection));
    if (ret != TRUE)
        g_error ("failed to remove Connection 0x%" PRIxPTR " from g_hash_table "
                 "0x%" PRIxPTR "using key 0x%" PRIxPTR, (uintptr_t)connection,
                 (uintptr_t)manager->connection_from_fd_table,
                 (uintptr_t)connection_key_fd (connection));
    ret = g_hash_table_remove (manager->connection_from_id_table,
                               connection_key_id (connection));
    if (ret != TRUE)
        g_error ("failed to remove Connection 0x%" PRIxPTR " from g_hash_table "
                 "0x%" PRIxPTR " using key %" PRIxPTR, (uintptr_t)connection,
                 (uintptr_t)manager->connection_from_fd_table,
                 (uintptr_t)connection_key_id (connection));
    pthread_mutex_unlock (&manager->mutex);

    return ret;
}

void
set_fd (gpointer key,
        gpointer value,
        gpointer user_data)
{
    fd_set *fds = (fd_set*)user_data;
    Connection *connection = CONNECTION (value);

    FD_SET (connection_receive_fd (connection), fds);
}

void
connection_manager_set_fds (ConnectionManager   *manager,
                            fd_set              *fds)
{
    pthread_mutex_lock (&manager->mutex);
    g_hash_table_foreach (manager->connection_from_fd_table,
                          set_fd,
                          fds);
    pthread_mutex_unlock (&manager->mutex);
}

guint
connection_manager_size (ConnectionManager   *manager)
{
    return g_hash_table_size (manager->connection_from_fd_table);
}

gboolean
connection_manager_is_full (ConnectionManager *manager)
{
    guint table_size;

    table_size = g_hash_table_size (manager->connection_from_fd_table);
    if (table_size < manager->max_connections) {
        return FALSE;
    } else {
        return TRUE;
    }
}