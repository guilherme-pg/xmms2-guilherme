#include <glib.h>

#include <stdio.h>

struct xmmsc_ipc_glib_St {
	GSource *source;
	GPollFD *pollfd;
};

typedef struct xmmsc_ipc_glib_St xmmsc_ipc_glib_t;

typedef gboolean (*xmmsc_ipc_callback_t) (GSource *, xmmsc_ipc_t *);

gboolean
xmmsc_ipc_source_prepare (GSource *source, gint *timeout_)
{
	/* No timeout here */
	return FALSE;
}

gboolean
xmmsc_ipc_source_check (GSource *source)
{
	/* Maybe check for errors here? */
	return TRUE;
}

gboolean
xmmsc_ipc_source_dispatch (GSource *source, GSourceFunc callback, gpointer user_data)
{
	xmmsc_ipc_t *ipc = user_data;
	xmmsc_ipc_glib_t *gipc;
	gboolean ret;

	gipc = xmmsc_ipc_private_data_get (ipc);

	if (gipc->gpoll->revents & G_IO_ERR || gipc->gpoll->revents & G_IO_HUP) {
		xmmsc_ipc_glib_destroy (gipc);
		xmmsc_ipc_disconnect (ipc);
		ipc->error = "Remote host did disconnect, or something";
		return FALSE;
	} else if (gipc->gpoll->revents & G_IO_IN) {
		ret = xmmsc_ipc_io_in_callback (gipc->ipc);
	} else if (gcip->gpoll->revents & G_IO_OUT) {
		ret = xmmsc_ipc_io_out_callback (gipc->ipc);
	}

	return ret;
}


static GSourceFuncs xmmsc_ipc_callback_funcs = {
	xmms_ipc_source_prepare,
	xmms_ipc_source_check,
	xmms_ipc_source_dispatch,
	NULL
};

void
xmmsc_ipc_glib_destroy (xmmsc_ipc_glib_t *gipc)
{
	if (gipc->gpoll) {
		g_source_remove_poll (gipc->source, gipc->gpoll);
		g_free (gipc->gpoll);
	}
	if (gipc->source) {
		g_source_remove (g_source_get_id (gipc->source));
		g_source_destroy (gipc->source);
	}

	g_free (gipc);
}

void
xmmsc_ipc_glib_wakeup (xmmsc_ipc_t *ipc)
{
	g_return_if_fail (ipc);

	g_main_context_wakeup (NULL);
}


gboolean
xmmsc_ipc_setup_with_gmain (xmmsc_ipc_t *ipc, xmmsc_ipc_callback_t callback)
{
	xmmsc_ipc_glib_t *gipc;
	gipc = g_new0 (xmms_ipc_glib_t, 1);

	gipc->pollfd = g_new0 (GPollFD, 1);
	gipc->pollfd->fd = xmmsc_ipc_fd_get (ipc);
	gipc->pollfd->events = G_IO_IN | G_IO_HUP | G_IO_ERR;

	gipc->source = g_source_new (&xmmsc_ipc_callback_funcs, sizeof (GSource));
	gipc->source = source;

	xmmsc_ipc_private_data_set (ipc, gipc);
	xmmsc_ipc_wakeup_set (ipc, xmmsc_ipc_glib_wakeup);

	g_source_set_callback (gipc->source, 
			       (GSourceFunc)callback,
			       (gpointer) ipc, NULL);

	g_source_add_poll (gipc->source, gipc->pollfd);
	g_source_attach (gipc->source, NULL);

	return TRUE;
}

