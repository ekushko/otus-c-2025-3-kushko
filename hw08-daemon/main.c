#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <syslog.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/resource.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <glib.h>
#include <glib/gprintf.h>

#define APP_NAME "hw08-daemon"
#define LOCKFILE "./" APP_NAME ".pid"
#define CONFFILE "./" APP_NAME ".ini"
#define BUF_SIZE 1024
#define LOCKMODE (S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH)

int  sock;
char socket_file[256];

static void sigquit_handler() {
    close(sock);
    unlink(socket_file);
}

void daemonize(const char* cmd) {
    int fd0, fd1, fd2;
    pid_t pid;
    struct rlimit rl;
    struct sigaction sa;

    openlog(cmd, LOG_CONS, LOG_DAEMON);

    umask(0);

    if (getrlimit(RLIMIT_NOFILE, &rl) < 0)
        perror("unable to get descriptor's max number");

    if ((pid = fork()) < 0)
        perror("fork calling error");
    else if (pid != 0)
        exit(0);

    setsid();

//    sa.sa_handler = SIG_IGN;
//    sigemptyset(&sa.sa_mask);
//    sa.sa_flags = 0;

//    if (sigaction(SIGHUP, &sa, NULL) < 0)
//        syslog(LOG_CRIT, "unable to ignore SIGHUP signal");

    sa.sa_handler = sigquit_handler;
    sigfillset(&sa.sa_mask);
    sigdelset(&sa.sa_mask, SIGQUIT);
    sa.sa_flags = 0;
    if (sigaction(SIGQUIT, &sa, NULL) < 0) {
        syslog(LOG_ERR, "unable to intercept signal SIGQUIT: %s", strerror(errno));
        exit(0);
    }

    if ((pid = fork()) < 0)
        syslog(LOG_CRIT, "fork calling error");
    else if (pid != 0)
        exit(0);

    if (chdir("/") < 0)
        syslog(LOG_CRIT, "unable to set / for current working directory");

    if (rl.rlim_max == RLIM_INFINITY)
        rl.rlim_max = 1024;

    for (unsigned long i = 0; i < rl.rlim_max; i++)
        close(i);

    fd0 = open("/dev/null", O_RDWR);
    fd1 = dup(0);
    fd2 = dup(0);

    if (fd0 != 0 || fd1 != 1 || fd2 != 2)
        syslog(LOG_CRIT, "invalid file descriptors %d %d %d", fd0, fd1, fd2);
}

int lockfile(int fd) {
    struct flock fl;
    fl.l_type = F_WRLCK;
    fl.l_start = 0;
    fl.l_whence = SEEK_SET;
    fl.l_len = 0;
    return(fcntl(fd, F_SETLK, &fl));
}

int already_running(const char *lock_file) {
    int fd;
    char buf[16];

    fd = open(lock_file, O_RDWR|O_CREAT, LOCKMODE);
    if (fd < 0) {
        syslog(LOG_ERR, "unable to open %s: %s", lock_file, strerror(errno));
        exit(1);
    }

    if (lockfile(fd) < 0) {
        if (errno == EACCES || errno == EAGAIN) {
            close(fd);
            return 1;
        }
        syslog(LOG_ERR, "unable to set block to %s: %s", lock_file, strerror(errno));
        exit(1);
    }

    ftruncate(fd, 0);
    sprintf(buf, "%ld", (long)getpid());
    write(fd, buf, strlen(buf)+1);
    return 0;
}

int load_from_config_file(const char *config_file, char *target_file, char *socket_file, _Bool *should_daemonize) {
    g_autoptr (GError) error = NULL;
    g_autoptr (GKeyFile) key_file = g_key_file_new();

    if (!g_key_file_load_from_file(key_file, config_file, G_KEY_FILE_NONE, &error)) {
        if (!g_error_matches(error, G_FILE_ERROR, G_FILE_ERROR_NOENT))
            g_warning("Error loading key file: %s", error->message);
            return -1;
    }

    g_autofree gchar *g_lock_file = g_key_file_get_string(key_file, "Common", "LockFile", &error);
    if (g_lock_file == NULL && !g_error_matches(error, G_KEY_FILE_ERROR, G_KEY_FILE_ERROR_KEY_NOT_FOUND)) {
        g_warning("Error finding key in key file: %s", error->message);
        return -1;
    } else if (g_lock_file == NULL) {
        g_lock_file = g_strdup("./CMakeCache.txt");
    }

    strcpy(target_file, (char *)g_lock_file);

    g_autofree gchar *g_socket_file = g_key_file_get_string(key_file, "Common", "SocketFile", &error);
    if (g_socket_file == NULL && !g_error_matches (error, G_KEY_FILE_ERROR, G_KEY_FILE_ERROR_KEY_NOT_FOUND)) {
        g_warning("Error finding key in key file: %s", error->message);
        return -1;
    } else if (g_socket_file == NULL) {
        g_socket_file = g_strdup("/tmp/hw08-daemon.socket");
    }

    strcpy(socket_file, (char *)g_socket_file);

    g_autofree gchar *g_daemonize = g_key_file_get_string(key_file, "Common", "Daemonize", &error);
    if (g_daemonize == NULL && !g_error_matches (error, G_KEY_FILE_ERROR, G_KEY_FILE_ERROR_KEY_NOT_FOUND)) {
        g_warning("Error finding key in key file: %s", error->message);
        return -1;
    } else if (g_daemonize == NULL) {
        g_daemonize = g_strdup("no");
    }

    *should_daemonize = strcmp(g_daemonize, "yes") == 0 ? TRUE : FALSE;

    return 0;
}

void run_unix_socket(const char *target_file, const char *socket_file) {
    int sock, msgsock;
    struct sockaddr_un server;
    char buf[BUF_SIZE];
    struct stat stat_buf;

    sock = socket(AF_UNIX, SOCK_STREAM, 0);

    if (sock < 0) {
        perror("opening stream socket");
        exit(1);
    }

    server.sun_family = AF_UNIX;
    strcpy(server.sun_path, socket_file);

    if (bind(sock, (struct sockaddr *) &server, sizeof(struct sockaddr_un))) {
        perror("binding stream socket");
        exit(1);
    }

    printf("Socket has name %s\n", server.sun_path);
    listen(sock, 5);

    for (;;) {
        msgsock = accept(sock, 0, 0);
        if (msgsock == -1) {
            perror("accept");
            break;
        } else {
            if (stat(target_file, &stat_buf) < 0)
                snprintf(buf, BUF_SIZE, "stat error:%s file:%s\n", strerror(errno), target_file);
            else
                snprintf(buf, BUF_SIZE, "size of %s:%ld bytes\n", target_file, stat_buf.st_size);

           send(msgsock, buf, strlen(buf), 0);
        }
        close(msgsock);
    }

    close(sock);
    unlink(socket_file);
}

int main() {
    char target_file[256];
    _Bool should_daemonize;

    if (load_from_config_file(CONFFILE, target_file, socket_file, &should_daemonize) == 0) {
        if (should_daemonize == TRUE && already_running(LOCKFILE) == 0)
            daemonize(APP_NAME);

        run_unix_socket(target_file, socket_file);
    }

    return 0;
}
