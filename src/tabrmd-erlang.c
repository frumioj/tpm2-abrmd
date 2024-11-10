#include <erl_interface.h>
#include <ei.h>
#include "tabrmd-erlang.h"

struct _TabrmdErlangInterface {
    GObject parent;
    Tabrmd *tabrmd;
    GThread *port_thread;
    gboolean running;
};

G_DEFINE_TYPE (TabrmdErlangInterface, tabrmd_erlang_interface, G_TYPE_OBJECT);

// Message handlers that mirror D-Bus interface
static void
handle_start_session (TabrmdErlangInterface *iface, ei_x_buff *response)
{
    // Mirror functionality from tabrmd-dbus.c StartSession
    // Return session handle in Erlang format
}

static void
handle_end_session (TabrmdErlangInterface *iface, uint32_t handle, ei_x_buff *response)
{
    // Mirror functionality from tabrmd-dbus.c EndSession
}

static void
handle_transmit(TabrmdErlangInterface *iface, 
                const uint8_t *command, 
                size_t command_size,
                ei_x_buff *response)
{
    TSS2_RC rc;
    Connection *connection;
    uint8_t *reply;
    size_t reply_size;

    rc = access_broker_send_command(iface->tabrmd->access_broker,
                                  command,
                                  command_size,
                                  &reply,
                                  &reply_size);

    ei_x_new_with_version(response);
    if (rc == TSS2_RC_SUCCESS) {
        ei_x_encode_tuple_header(response, 2);
        ei_x_encode_atom(response, "ok");
        ei_x_encode_binary(response, reply, reply_size);
        g_free(reply);
    } else {
        ei_x_encode_tuple_header(response, 2);
        ei_x_encode_atom(response, "error");
        ei_x_encode_long(response, rc);
    }
}

static void
handle_cancel(TabrmdErlangInterface *iface,
             uint32_t handle,
             ei_x_buff *response)
{
    TSS2_RC rc;
    
    rc = resource_manager_cancel(iface->tabrmd->resource_manager, handle);
    
    ei_x_new_with_version(response);
    ei_x_encode_tuple_header(response, 2);
    ei_x_encode_atom(response, rc == TSS2_RC_SUCCESS ? "ok" : "error");
    ei_x_encode_long(response, rc);
}

static void
handle_set_locality(TabrmdErlangInterface *iface,
                   uint32_t handle,
                   uint8_t locality,
                   ei_x_buff *response)
{
    TSS2_RC rc;
    
    rc = resource_manager_set_locality(iface->tabrmd->resource_manager,
                                     handle,
                                     locality);
    
    ei_x_new_with_version(response);
    ei_x_encode_tuple_header(response, 2);
    ei_x_encode_atom(response, rc == TSS2_RC_SUCCESS ? "ok" : "error");
    ei_x_encode_long(response, rc);
}

static gpointer
port_thread_func (gpointer data)
{
    TabrmdErlangInterface *iface = TABRMD_ERLANG_INTERFACE(data);
    ei_x_buff response;
    
    while (iface->running) {
        // Read command from Erlang port
        // Parse command and parameters
        // Call appropriate handler
        // Send response back through port
    }
    
    return NULL;
}

TabrmdErlangInterface*
tabrmd_erlang_interface_new (Tabrmd *tabrmd)
{
    TabrmdErlangInterface *iface;
    
    iface = g_object_new (TABRMD_TYPE_ERLANG_INTERFACE, NULL);
    iface->tabrmd = tabrmd;
    
    return iface;
}

void
tabrmd_erlang_interface_start (TabrmdErlangInterface *iface)
{
    iface->running = TRUE;
    iface->port_thread = g_thread_new ("erlang-port",
                                     port_thread_func,
                                     iface);
}

void
tabrmd_erlang_interface_stop (TabrmdErlangInterface *iface)
{
    iface->running = FALSE;
    g_thread_join (iface->port_thread);
} 