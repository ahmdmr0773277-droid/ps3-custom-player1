#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sysutil/sysutil.h>
#include <io/pad.h>

// مسارات ملف تسجيل الأخطاء
#define LOG_USB "/dev_usb000/player_errors.txt"
#define LOG_HDD "/dev_hdd0/game/CNMN00001/USRDIR/player_errors.txt"
#define URL_FILE "USRDIR/url.txt"

// دالة كتابة الأخطاء في ملف خارجي
void write_log(const char *format, ...) {
    // نحاول نكتب على الفلاش ميموري أولاً حتى يسهل عليك تقرأ الخطأ بالكمبيوتر
    FILE *f = fopen(LOG_USB, "a");
    if (!f) {
        // إذا ماكو فلاش ميموري، يكتب بالذاكرة الداخلية للعبة
        f = fopen(LOG_HDD, "a");
    }
    
    if (f) {
        time_t t = time(NULL);
        struct tm *tm_info = localtime(&t);
        char time_str[26];
        strftime(time_str, 26, "%Y-%m-%d %H:%M:%S", tm_info);
        
        fprintf(f, "[%s] ", time_str);
        
        va_list args;
        va_start(args, format);
        vfprintf(f, format, args);
        va_end(args);
        
        fprintf(f, "\n");
        fclose(f);
    }
}

// دالة فحص وتحليل رابط البث
int connect_to_stream(const char* url) {
    write_log("INFO: Attempting to parse URL: %s", url);
    
    // فحص مبدئي للرابط
    if (strncmp(url, "http://", 7) != 0 && strncmp(url, "https://", 8) != 0) {
        write_log("ERROR: Invalid protocol. Only HTTP/HTTPS streams are supported.");
        return -1;
    }

    char host[256];
    int port = 80;
    // استخراج الهوست والمنفذ (بشكل مبسط)
    const char* host_start = strstr(url, "://") + 3;
    const char* port_start = strchr(host_start, ':');
    const char* path_start = strchr(host_start, '/');

    if (port_start && (!path_start || port_start < path_start)) {
        sscanf(host_start, "%255[^:]:%d", host, &port);
    } else if (path_start) {
        sscanf(host_start, "%255[^/]", host);
    } else {
        strcpy(host, host_start);
    }

    write_log("INFO: Connecting to host: %s on port: %d", host, port);

    // إنشاء اتصال الشبكة (Socket)
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        write_log("ERROR: Failed to create socket system object.");
        return -1;
    }

    struct hostent *server = gethostbyname(host);
    if (server == NULL) {
        write_log("ERROR: DNS resolution failed. Cannot find host: %s (Check Internet Connection)", host);
        close(sock);
        return -1;
    }

    struct sockaddr_in serv_addr;
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    memcpy(&serv_addr.sin_addr.s_addr, server->h_addr, server->h_length);
    serv_addr.sin_port = htons(port);

    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        write_log("ERROR: TCP Connection failed to %s:%d. Server might be down.", host, port);
        close(sock);
        return -1;
    }

    write_log("SUCCESS: Connected to stream server successfully!");
    close(sock);
    return 0;
}

int main(int argc, char* argv[]) {
    // تهيئة نظام عرض الـ PS3
    // هنا نضع كود تهيئة الشاشة الافتراضي لـ PSL1GHT

    write_log("SYSTEM: Application started successfully.");

    // 1. محاولة قراءة ملف الرابط
    FILE *url_file = fopen(URL_FILE, "r");
    if (!url_file) {
        write_log("ERROR: Could not open '%s'. Make sure the file exists in USRDIR.", URL_FILE);
        return 0;
    }

    char stream_url[512];
    if (fgets(stream_url, sizeof(stream_url), url_file) != NULL) {
        // إزالة الفراغات وأسطر النزول الجديدة
        stream_url[strcspn(stream_url, "\r\n")] = 0;
        
        // محاولة تشغيل والاتصال بالرابط
        connect_to_stream(stream_url);
    } else {
        write_log("ERROR: '%s' is empty. Please write a valid stream link.", URL_FILE);
    }
    fclose(url_file);

    // حلقة برمجية لإبقاء التطبيق يعمل حتى تضغط زر الخروج (دائرة)
    PadInfo padinfo;
    PadData paddata;
    while(1) {
        ioPadGetInfo(&padinfo);
        if(padinfo.status[0]) {
            ioPadGetData(0, &paddata);
            if(paddata.BTN_CIRCLE) {
                write_log("SYSTEM: Exit button pressed. Closing application.");
                break;
            }
        }
        usleep(16000); // 60 FPS update
    }

    return 0;
}