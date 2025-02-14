

/* qt会将glib里的signals成员识别为宏，所以取消该宏
 * 后面如果用到signals时，使用Q_SIGNALS代替即可
 **/
#ifdef signals
#undef signals
#endif

extern "C" {
#include <glib.h>
#include <gio/gio.h>

#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sys/wait.h>

}

#include "run-passwd2.h"

/* Buffer size for backend output */
#define BUFSIZE 64

/* Passwd states */
//后端passwd的状态，NONE应该是passwd还没有启动，ERROR表示报错但还没退出
typedef enum {
        PASSWD_STATE_NONE,              /* Passwd is not asking for anything */
        PASSWD_STATE_AUTH,              /* Passwd is asking for our current password */
        PASSWD_STATE_NEW,               /* Passwd is asking for our new password */
        PASSWD_STATE_RETYPE,            /* Passwd is asking for our retyped new password */
        PASSWD_STATE_ERR                /* Passwd reported an error but has not yet exited */
} PasswdState;

struct PasswdHandler {
//        GtkBuilder  *ui;

        const char *current_password;
        const char *new_password;
        const char *retyped_password;

        /* Communication with the passwd program */
        GPid backend_pid;

        GIOChannel *backend_stdin;
        GIOChannel *backend_stdout;

        GQueue *backend_stdin_queue;            /* Write queue to backend_stdin */

        /* GMainLoop IDs */
        guint backend_child_watch_id;           /* g_child_watch_add (PID) */
        guint backend_stdout_watch_id;          /* g_io_add_watch (stdout) */

        /* State of the passwd program */
        PasswdState backend_state;
        gboolean    changing_password;

        PasswdCallback auth_cb;
        gpointer       auth_cb_data;

        PasswdCallback chpasswd_cb;
        gpointer       chpasswd_cb_data;
};

//GQuark是一个guint32
static GQuark
passwd_error_quark (void)
{
    static GQuark q = 0;

    //返回错误的标识码
    if (q == 0) {
        q = g_quark_from_static_string("passwd_error");
    }

    return q;
}

/* Error handling */
#define PASSWD_ERROR (passwd_error_quark ())

static void stop_passwd (PasswdHandler *passwd_handler);

static void free_passwd_resources (PasswdHandler *passwd_handler);

static gboolean io_watch_stdout (GIOChannel *source, GIOCondition condition, PasswdHandler *passwd_handler);


static void free_passwd_resources (PasswdHandler *passwd_handler)
{
    GError  *error = NULL;

    /* Remove the child watcher */
    if (passwd_handler->backend_child_watch_id != 0) {

        g_source_remove (passwd_handler->backend_child_watch_id);

        passwd_handler->backend_child_watch_id = 0;
    }


    /* Close IO channels (internal file descriptors are automatically closed) */
    if (passwd_handler->backend_stdin != NULL) {

        if (g_io_channel_shutdown (passwd_handler->backend_stdin, TRUE, &error) != G_IO_STATUS_NORMAL) {
            g_warning ("Could not shutdown backend_stdin IO channel: %s", error->message);
            g_error_free (error);
            error = NULL;
        }

        g_io_channel_unref (passwd_handler->backend_stdin);
        passwd_handler->backend_stdin = NULL;
    }

    if (passwd_handler->backend_stdout != NULL) {

        if (g_io_channel_shutdown (passwd_handler->backend_stdout, TRUE, &error) != G_IO_STATUS_NORMAL) {
            g_warning ("Could not shutdown backend_stdout IO channel: %s", error->message);
            g_error_free (error);
            error = NULL;
        }

        g_io_channel_unref (passwd_handler->backend_stdout);

        passwd_handler->backend_stdout = NULL;
    }

    /* Remove IO watcher */
    if (passwd_handler->backend_stdout_watch_id != 0) {

        g_source_remove (passwd_handler->backend_stdout_watch_id);

        passwd_handler->backend_stdout_watch_id = 0;
    }

    /* Close PID */
    //因为flag为G_SPAWN_DO_NOT_REAP_CHILD,所以child不会自动的被reap掉，需要在子进程上free
    if (passwd_handler->backend_pid != -1) {

        g_spawn_close_pid (passwd_handler->backend_pid);

        passwd_handler->backend_pid = -1;
    }

    /* Clear backend state */
    passwd_handler->backend_state = PASSWD_STATE_NONE;
}

static void authenticate (PasswdHandler *passwd_handler)
{
        gchar   *s;

        s = g_strdup_printf ("%s\n", passwd_handler->current_password);
        g_queue_push_tail (passwd_handler->backend_stdin_queue, s);
}

static void io_queue_pop (GQueue *queue, GIOChannel *channel)
{
    gchar   *buf;
    gsize   bytes_written;
    GError  *error = NULL;

    buf = (gchar *)g_queue_pop_head (queue);

    if (buf != NULL) {
        //将队列中的首元素写入到channel中
        if (g_io_channel_write_chars (channel, buf, -1, &bytes_written, &error) != G_IO_STATUS_NORMAL) {
            g_warning ("Could not write queue element \"%s\" to channel: %s", buf, error->message);
            g_error_free (error);
        }

        /* Ensure passwords are cleared from memory */
        //清除内存中的passwords
        memset (buf, 0, strlen (buf));
        g_free (buf);
    }

}

static gboolean is_string_complete (gchar *str, ...)
{
    va_list ap;
    gchar   *arg;

    if (strlen (str) == 0) {
        return FALSE;
    }

    va_start (ap, str);

    while ((arg = va_arg (ap, char *)) != NULL) {
        if (g_strrstr (str, arg) != NULL) {
            va_end (ap);
            return TRUE;
        }
    }

    va_end (ap);

    return FALSE;
}

static gboolean io_watch_stdout (GIOChannel *source, GIOCondition condition, PasswdHandler *passwd_handler)
{
    static GString *str = NULL;     /* Persistent buffer */

    gchar           buf[BUFSIZE];           /* Temporary buffer */
    gsize           bytes_read;
    GError          *gio_error = NULL;      /* Error returned by functions */
    GError          *error = NULL;          /* Error sent to callbacks */

    //GtkBuilder      *dialog;

    gboolean        reinit = FALSE;

    /* Initialize buffer */
    if (str == NULL) {
        str = g_string_new ("");
    }

    //dialog = passwd_handler->ui;
    //buf将保存从channel中读取到的数据,bytes_read表示从buf中读取的数据长度
    if (g_io_channel_read_chars (source, buf, BUFSIZE, &bytes_read, &gio_error)
            != G_IO_STATUS_NORMAL) {
        g_warning ("IO Channel read error: %s", gio_error->message);
        g_error_free (gio_error);

        return TRUE;
    }

    //		g_warning("----------bytes_read=%d",bytes_read);
    //		g_warning("----------io_watch_buf=%s-------",buf);

    str = g_string_append_len (str, buf, bytes_read);

    /* In which state is the backend? */
    switch (passwd_handler->backend_state) {
    case PASSWD_STATE_AUTH:
        /* Passwd is asking for our current password */

        if (is_string_complete (str->str, "assword: ", "failure", "wrong", "error", NULL)) {

            if (g_strrstr (str->str, "assword: ") != NULL) {
                /* Authentication successful */

                passwd_handler->backend_state = PASSWD_STATE_NEW;

                /* Trigger callback to update authentication status */
                if (passwd_handler->auth_cb)
                    passwd_handler->auth_cb (passwd_handler,
                                             NULL,
                                             passwd_handler->auth_cb_data);

            } else {
                /* Authentication failed */

                error = g_error_new_literal (PASSWD_ERROR, PASSWD_ERROR_AUTH_FAILED,
                                             "Authentication failure!");

                passwd_handler->changing_password = FALSE;

                /* This error can happen both while authenticating or while changing password:
                                         * if chpasswd_cb is set, this means we're already changing password */
                if (passwd_handler->chpasswd_cb)
                    passwd_handler->chpasswd_cb (passwd_handler,
                                                 error,
                                                 passwd_handler->auth_cb_data);
                else if (passwd_handler->auth_cb)
                    passwd_handler->auth_cb (passwd_handler,
                                             error,
                                             passwd_handler->auth_cb_data);

                g_error_free (error);
            }

            reinit = TRUE;
        }
        break;
    case PASSWD_STATE_NEW:
        /* Passwd is asking for our new password */

        if (is_string_complete (str->str, "assword: ", NULL)) {
            /* Advance to next state */
            passwd_handler->backend_state = PASSWD_STATE_RETYPE;

            /* Pop retyped password from queue and into IO channel */
            io_queue_pop (passwd_handler->backend_stdin_queue, passwd_handler->backend_stdin);

            reinit = TRUE;
        }
        break;
    case PASSWD_STATE_RETYPE:
        /* Passwd is asking for our retyped new password */

        //                        if (is_string_complete (str->str,
        //                                                "successfully",
        //                                                "short",
        //                                                "longer",
        //                                                "palindrome",
        //                                                "dictionary",
        //                                                "simple",
        //                                                "simplistic",
        //                                                "similar",
        //                                                "different",
        //                                                "case",
        //                                                "wrapped",
        //                                                "recovered",
        //                                                "recent",
        //                                                "unchanged",
        //                                                "match",
        //                                                "1 numeric or special",
        //                                                "failure",
        //                                                "length",
        //                                                NULL)) {
        if (TRUE){

            if (g_strrstr (str->str, "successfully") != NULL) {
                /* Hooray! */

                /* Trigger callback to update status */
                if (passwd_handler->chpasswd_cb)
                    passwd_handler->chpasswd_cb (passwd_handler,
                                                 NULL,
                                                 passwd_handler->chpasswd_cb_data);
            }
            else {
                /* Ohnoes! */

                if (g_strrstr (str->str, "recovered") != NULL) {
                    /* What does this indicate?
                                                 * "Authentication information cannot be recovered?" from libpam? */
                    error = g_error_new_literal (PASSWD_ERROR, PASSWD_ERROR_UNKNOWN,
                                                 str->str);
                } else if (g_strrstr (str->str, "short") != NULL ||
                           g_strrstr (str->str, "longer") != NULL) {
                    error = g_error_new (PASSWD_ERROR, PASSWD_ERROR_REJECTED,
                                         "New password length is too short!");
                } else if (g_strrstr (str->str, "palindrome") != NULL ||
                           g_strrstr (str->str, "simple") != NULL ||
                           g_strrstr (str->str, "simplistic") != NULL ||
                           g_strrstr (str->str, "dictionary") != NULL) {
                    error = g_error_new (PASSWD_ERROR, PASSWD_ERROR_REJECTED,
                                         "The new password is too simple!");
                } else if (g_strrstr (str->str, "similar") != NULL ||
                           g_strrstr (str->str, "different") != NULL ||
                           g_strrstr (str->str, "case") != NULL ||
                           g_strrstr (str->str, "wrapped") != NULL) {
                    error = g_error_new (PASSWD_ERROR, PASSWD_ERROR_REJECTED,
                                         "The new password is too similar to the old one!");
                } else if (g_strrstr (str->str, "1 numeric or special") != NULL) {
                    error = g_error_new (PASSWD_ERROR, PASSWD_ERROR_REJECTED,
                                         "The new password must contain numbers or special characters!");
                } else if (g_strrstr (str->str, "unchanged") != NULL ||
                           g_strrstr (str->str, "match") != NULL) {
                    error = g_error_new (PASSWD_ERROR, PASSWD_ERROR_REJECTED,
                                         "The new password is the same as the old one!");
                } else if (g_strrstr (str->str, "recent") != NULL) {
                    error = g_error_new (PASSWD_ERROR, PASSWD_ERROR_REJECTED,
                                         "The new password has been used recently!");
                } else if (g_strrstr (str->str, "failure") != NULL) {
                    /* Authentication failure */
                    error = g_error_new (PASSWD_ERROR, PASSWD_ERROR_AUTH_FAILED,
                                         "Your password has been changed after you verify!");
                }
                else {
                    error = g_error_new (PASSWD_ERROR, PASSWD_ERROR_UNKNOWN,
                                         "Unknown error");
                }

                /* At this point, passwd might have exited, in which case
                                         * child_watch_cb should clean up for us and remove this watcher.
                                         * On some error conditions though, passwd just re-prompts us
                                         * for our new password. */
                passwd_handler->backend_state = PASSWD_STATE_ERR;

                passwd_handler->changing_password = FALSE;

                /* Trigger callback to update status */
                if (passwd_handler->chpasswd_cb)
                    passwd_handler->chpasswd_cb (passwd_handler,
                                                 error,
                                                 passwd_handler->chpasswd_cb_data);

                g_error_free (error);

            }

            reinit = TRUE;

            /* child_watch_cb should clean up for us now */
        }
        break;
    case PASSWD_STATE_NONE:
        /* Passwd is not asking for anything yet */
        if (is_string_complete (str->str, "assword: ", NULL)) {

            /* If the user does not have a password set,
                                 * passwd will immediately ask for the new password,
                                 * so skip the AUTH phase */
            if (is_string_complete (str->str, "new", "New", NULL)) {
                gchar *pw;

                passwd_handler->backend_state = PASSWD_STATE_NEW;

                /* since passwd didn't ask for our old password
                                         * in this case, simply remove it from the queue */
                pw = (gchar *)g_queue_pop_head (passwd_handler->backend_stdin_queue);
                g_free (pw);

                /* Pop the IO queue, i.e. send new password */
                io_queue_pop (passwd_handler->backend_stdin_queue, passwd_handler->backend_stdin);
            } else {

                passwd_handler->backend_state = PASSWD_STATE_AUTH;

                /* Pop the IO queue, i.e. send current password */
                io_queue_pop (passwd_handler->backend_stdin_queue, passwd_handler->backend_stdin);
            }

            reinit = TRUE;
        }
        break;
    default:
        /* Passwd has returned an error */
        reinit = TRUE;
        break;
    }

    if (reinit) {
        g_string_free (str, TRUE);
        str = NULL;
    }

    /* Continue calling us */
    return TRUE;
}

/* Child watcher */
static void child_watch_cb (GPid pid, gint status, PasswdHandler *passwd_handler)
{
    //子进程正常结束为非0
    if (WIFEXITED (status)) {
        //取得子进程正常退出时返回的结束代码
        if (WEXITSTATUS (status) >= 255) {
            g_warning ("Child exited unexpectedly");
        }
    }

    free_passwd_resources (passwd_handler);
}

static void stop_passwd (PasswdHandler *passwd_handler)
{
    /* This is the standard way of returning from the dialog with passwd.
     * If we return this way we can safely kill passwd as it has completed
     * its task.
     */

    if (passwd_handler->backend_pid != -1) {
        kill (passwd_handler->backend_pid, 9);
    }

    /* We must run free_passwd_resources here and not let our child
     * watcher do it, since it will access invalid memory after the
     * dialog has been closed and cleaned up.
     *
     * If we had more than a single thread we'd need to remove
     * the child watch before trying to kill the child.
     */
    free_passwd_resources (passwd_handler);
}

static gboolean spawn_passwd (PasswdHandler *passwd_handler, const char * user_name, GError **error)
{
    gchar   *argv[3];
    gchar   *envp[1];
    gint    my_stdin, my_stdout, my_stderr;

    argv[0] = "/usr/bin/passwd";    /* Is it safe to rely on a hard-coded path? */
    argv[1] = g_strdup_printf ("%s", user_name);
    argv[2] = NULL;

//    g_warning("spawn_passwd: %s %s", argv[0], argv[1]);

    envp[0] = NULL;                 /* If we pass an empty array as the environment,
                                         * will the childs environment be empty, and the
                                         * locales set to the C default? From the manual:
                                         * "If envp is NULL, the child inherits its
                                         * parent'senvironment."
                                         * If I'm wrong here, we somehow have to set
                                         * the locales here.
                                         */

    //创建一个管道,进行通信,子进程执行passwd命令
    if (!g_spawn_async_with_pipes (NULL,                            /* Working directory */
                                   argv,                            /* Argument vector */
                                   envp,                            /* Environment */
                                   G_SPAWN_DO_NOT_REAP_CHILD,       /* Flags */
                                   NULL,                            /* Child setup （在子进程调用exec()之前，该函数会被调用）*/
                                   NULL,                            /* Data to child setup */
                                   &passwd_handler->backend_pid,    /* PID */
                                   &my_stdin,                       /* Stdin */
                                   &my_stdout,                      /* Stdout */
                                   &my_stderr,                      /* Stderr */
                                   error)) {                        /* GError */

        /* An error occured */
        free_passwd_resources (passwd_handler);

        return FALSE;
    }

    /* 2>&1 */
    //复制文件描述符，也就是将stderr重定向到stdout
    if (dup2 (my_stderr, my_stdout) == -1) {
        /* Failed! */
        g_set_error_literal (error,
                             PASSWD_ERROR,
                             PASSWD_ERROR_BACKEND,
                             strerror (errno));

        /* Clean up */
        stop_passwd (passwd_handler);

        return FALSE;
    }

    /* Open IO Channels */
    //指定一个文件描述符，创建一个IO Channel，默认使用UTF-8编码格式
    passwd_handler->backend_stdin = g_io_channel_unix_new (my_stdin);
    passwd_handler->backend_stdout = g_io_channel_unix_new (my_stdout);

    /* Set raw encoding */
    /* Set nonblocking mode */
    //设置通道的编码方式为NULL,设置为非阻塞的方式
    if (g_io_channel_set_encoding (passwd_handler->backend_stdin, NULL, error) != G_IO_STATUS_NORMAL ||
            g_io_channel_set_encoding (passwd_handler->backend_stdout, NULL, error) != G_IO_STATUS_NORMAL ||
            g_io_channel_set_flags (passwd_handler->backend_stdin, G_IO_FLAG_NONBLOCK, error) != G_IO_STATUS_NORMAL ||
            g_io_channel_set_flags (passwd_handler->backend_stdout, G_IO_FLAG_NONBLOCK, error) != G_IO_STATUS_NORMAL ) {

        /* Clean up */
        stop_passwd (passwd_handler);
        return FALSE;
    }

    /* Turn off buffering */
    //只有通道的编码方式为NULL，才能设置缓冲状态为FASLE，其他任何编码，通道必须被缓冲，这里是为了清掉上次的密码
    g_io_channel_set_buffered (passwd_handler->backend_stdin, FALSE);
    g_io_channel_set_buffered (passwd_handler->backend_stdout, FALSE);

    /* Add IO Channel watcher */
    //当IO通道的状态为G_IO_IN(从IO通道读数据时)或者G_IO_PRI(读紧急数据时)时，调用io_watch_stdout
    passwd_handler->backend_stdout_watch_id = g_io_add_watch (passwd_handler->backend_stdout,
                                                              G_IO_IN /*| G_IO_PRI*/,
                                                              (GIOFunc) io_watch_stdout, passwd_handler);

    /* Add child watcher */
    //在指定pid的进程退出时，调用child_watch_cb(),进行错误检查,以及资源回收
    passwd_handler->backend_child_watch_id = g_child_watch_add (passwd_handler->backend_pid, (GChildWatchFunc) child_watch_cb, passwd_handler);

    /* Success! */

    return TRUE;
}

static void update_password (PasswdHandler *passwd_handler)
{
        gchar   *s;

        s = g_strdup_printf ("%s\n", passwd_handler->new_password);

        g_queue_push_tail (passwd_handler->backend_stdin_queue, s);
        /* We need to allocate new space because io_queue_pop() g_free()s
         * every element of the queue after it's done */
        g_queue_push_tail (passwd_handler->backend_stdin_queue, g_strdup (s));
}

gboolean passwd_change_password (PasswdHandler *passwd_handler, const char *user_name,
                        const char    *new_password,
                        PasswdCallback cb,
                        const gpointer user_data)
{
    GError *error = NULL;

    passwd_handler->changing_password = TRUE;

    passwd_handler->new_password = new_password;
    passwd_handler->chpasswd_cb = cb;
    passwd_handler->chpasswd_cb_data = user_data;

    /* Stop passwd if an error occured and it is still running */
    if (passwd_handler->backend_state == PASSWD_STATE_ERR) {

        /* Stop passwd, free resources */
        stop_passwd (passwd_handler);
    }

    /* Check that the backend is still running, or that an error
         * has occured but it has not yet exited */

    g_warning("passwd pid is: %d", passwd_handler->backend_pid);
    if (passwd_handler->backend_pid == -1) {
        /* If it is not, re-run authentication */

        /* Spawn backend */
        stop_passwd (passwd_handler);

        if (!spawn_passwd (passwd_handler, user_name, &error)) {
            g_error_free (error);

            return FALSE;
        }

        g_warning("------------1----------------------");

        /* Add current and new passwords to queue */
        //将当前的密码和新密码入队，新密码会入队两次
        authenticate (passwd_handler);
        update_password (passwd_handler);
    } else {
        g_warning("-----2-----------");
        /* Only add new passwords to queue */
        update_password (passwd_handler);
    }

    /* Pop new password through the backend. If user has no password, popping the queue
           would output current password, while 'passwd' is waiting for the new one. So wait
           for io_watch_stdout() to remove current password from the queue, and output
           the new one for us.*/
    //如果密码为空，将新进队列的密码，作为current_passwd弹出
    if (passwd_handler->current_password)
    {
        io_queue_pop (passwd_handler->backend_stdin_queue, passwd_handler->backend_stdin);
    }

    /* Our IO watcher should now handle the rest */

    return TRUE;
}

//void passwd_authenticate (PasswdHandler *passwd_handler,
//                     const char    *current_password,
//                     PasswdCallback cb,
//                     const gpointer user_data)
//{
//        GError *error = NULL;

//        /* Don't stop if we've already started chaging password */
//        if (passwd_handler->changing_password)
//                return;

//        /* Clear data from possible previous attempts to change password */
//        passwd_handler->new_password = NULL;
//        passwd_handler->chpasswd_cb = NULL;
//        passwd_handler->chpasswd_cb_data = NULL;
//        g_queue_foreach (passwd_handler->backend_stdin_queue, (GFunc) g_free, NULL);
//        g_queue_clear (passwd_handler->backend_stdin_queue);

//        passwd_handler->current_password = current_password;
//        passwd_handler->auth_cb = cb;
//        passwd_handler->auth_cb_data = user_data;

//        /* Spawn backend */
//        //重新启动后台passwd
//        stop_passwd (passwd_handler);

//        if (!spawn_passwd (passwd_handler, &error)) {
//                g_warning ("%s", error->message);
//                g_error_free (error);
//                return;
//        }

//        //将current passwd从尾部插入队列
//        authenticate (passwd_handler);

//        /* Our IO watcher should now handle the rest */
//}


PasswdHandler * passwd_init ()
{
    PasswdHandler *passwd_handler;

    passwd_handler = g_new0 (PasswdHandler, 1);

    /* Initialize backend_pid. -1 means the backend is not running */
    //-1代表后台还没启动
    passwd_handler->backend_pid = -1;

    /* Initialize IO Channels */
    passwd_handler->backend_stdin = NULL;
    passwd_handler->backend_stdout = NULL;

    /* Initialize write queue */
    passwd_handler->backend_stdin_queue = g_queue_new ();

    /* Initialize watchers */
    passwd_handler->backend_child_watch_id = 0;
    passwd_handler->backend_stdout_watch_id = 0;

    /* Initialize backend state */
    passwd_handler->backend_state = PASSWD_STATE_NONE;
    passwd_handler->changing_password = FALSE;

    return passwd_handler;
}


void passwd_destroy (PasswdHandler *passwd_handler)
{
    g_queue_free (passwd_handler->backend_stdin_queue);
    stop_passwd (passwd_handler);
    g_free (passwd_handler);
}
