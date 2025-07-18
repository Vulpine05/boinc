// This file is part of BOINC.
// http://boinc.berkeley.edu
// Copyright (C) 2008 University of California
//
// BOINC is free software; you can redistribute it and/or modify it
// under the terms of the GNU Lesser General Public License
// as published by the Free Software Foundation,
// either version 3 of the License, or (at your option) any later version.
//
// BOINC is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
// See the GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with BOINC.  If not, see <http://www.gnu.org/licenses/>.

#ifdef _WIN32
#include "boinc_win.h"
#else
#include "config.h"
#include <cstdarg>
#include <cstring>
#include <deque>
#endif
#include "str_util.h"

using std::deque;

#include "diagnostics.h"
#include "log_flags.h"
#include "str_replace.h"

#include "client_types.h"
#include "client_state.h"
#include "cs_notice.h"
#include "main.h"

#include "client_msgs.h"

MESSAGE_DESCS message_descs;

#ifdef SIM
extern void show_message(
    PROJ_AM *p, char* msg, int priority, bool is_html, const char* link
);
#else
// Show a message:
// 1) As a MESSAGE_DESC (for GUI event log)
// 2) As a NOTICE, if high priority (for GUI notices)
// 3) write to log file (stdoutdae.txt)
//
void show_message(
    PROJ_AM *p, char* msg, int priority, bool is_html, const char* link
) {
    const char* x;
    char message[1024], event_msg[2048], evt_message[2048];
    double t = dtime();
    char* time_string = time_to_string(t);

    // Cycle the log files if needed
    //
    diagnostics_cycle_logs();

    safe_strcpy(message, msg);
    strip_whitespace(message);

    // add a message
    //
    switch (priority) {
    case MSG_INTERNAL_ERROR:
        snprintf(event_msg, sizeof(event_msg), "[error] %s", message);
        break;
    case MSG_SCHEDULER_ALERT:
        snprintf(event_msg, sizeof(event_msg), "%.64s: %s",
            _("Message from server"), message
        );
        break;
    default:
        strlcpy(event_msg, message, sizeof(event_msg));
    }

    // The event log doesn't display HTML, so strip tags
    // The only case of this is
    // A new version of BOINC is available (8.0.2). <a href=https://boinc.berkeley.edu/download.php>Download</a>
    // so do it in a crude way.
    //
    if (is_html) {
        char *q = strchr(event_msg, '<');
        if (q) {
            *q = 0;
            strip_whitespace(event_msg);
        }
    }
    message_descs.insert(p, priority, (int)t, event_msg);

    // add a notice if needed
    //
    switch (priority) {
    case MSG_USER_ALERT:
    case MSG_SCHEDULER_ALERT:
        char buf[1024];
        if (is_html) {
            safe_strcpy(buf, message);
        } else {
            xml_escape(message, buf, sizeof(message));
        }
        NOTICE n;
        n.description = buf;
        if (link) {
            safe_strcpy(n.link, link);
        }
        if (p) {
            safe_strcpy(n.project_name, p->get_project_name());
        }
        n.create_time = n.arrival_time = t;
        safe_strcpy(n.category, (priority==MSG_USER_ALERT)?"client":"scheduler");
        notices.append(n);
    }

    strip_translation(message);

    if (p) {
        x = p->get_project_name();
    } else {
        x = "---";
    }

    // Construct message to be logged/displayed
    snprintf(evt_message, sizeof(evt_message), "%s [%s] %s\n", time_string,  x, message);

    // print message to the console
    printf("%s", evt_message);

#ifdef _WIN32
    // MSVCRT doesn't support line buffered streams
    fflush(stdout);
#endif

    // print message to the debugger view port
    diagnostics_trace_to_debugger(evt_message);
}
#endif

// Takes a printf style formatted string, inserts the proper values,
// and passes it to show_message
//
void msg_printf(PROJ_AM *p, int priority, const char *fmt, ...) {
    char buf[8192];  // output can be much longer than format
    va_list ap;

    if (fmt == NULL) return;

    va_start(ap, fmt);
    vsnprintf(buf, sizeof(buf), fmt, ap);
    buf[sizeof(buf)-1] = 0;
    va_end(ap);

    show_message(p, buf, priority, true, 0);
}

void msg_printf_notice(
    PROJ_AM *p,
    bool is_html,       // msg has HTML tags; don't XML-escape it
    const char* link,   // where 'more info' goes to
    const char *fmt, ...
) {
    char buf[8192];  // output can be much longer than format
    va_list ap;

    if (fmt == NULL) return;

    va_start(ap, fmt);
    vsnprintf(buf, sizeof(buf), fmt, ap);
    buf[sizeof(buf)-1] = 0;
    va_end(ap);

    show_message(p, buf, MSG_USER_ALERT, is_html, link);
}

// handle new message:
// add to cache, and delete old messages if cache too big.
// If high priority, create a notice.
//
void MESSAGE_DESCS::insert(PROJ_AM* p, int priority, int now, char* message) {
    MESSAGE_DESC* mdp = new MESSAGE_DESC;
    static int seqno = 1;
    if (p) {
        strlcpy(
            mdp->project_name, p->get_project_name(), sizeof(mdp->project_name)
        );
    } else {
        safe_strcpy(mdp->project_name, "");
    }
    mdp->priority = (priority==MSG_SCHEDULER_ALERT)?MSG_USER_ALERT:priority;
    mdp->timestamp = now;
    mdp->seqno = seqno++;
    mdp->message = message;
    while (msgs.size() > MAX_SAVED_MESSAGES) {
        delete msgs.back();
        msgs.pop_back();
    }
    msgs.push_front(mdp);
}

void MESSAGE_DESCS::write(int seqno, MIOFILE& fout, bool translatable) {
    int i, j;
    unsigned int k;
    MESSAGE_DESC* mdp;
    char buf[1024];

    // messages are stored in descreasing seqno,
    // i.e. newer ones are at the head of the vector.
    // compute j = index of first message to return
    //
    j = (int)msgs.size()-1;
    for (k=0; k<msgs.size(); k++) {
        mdp = msgs[k];
        if (mdp->seqno <= seqno) {
            j = k-1;
            break;
        }
    }

    fout.printf("<msgs>\n");
    for (i=j; i>=0; i--) {
        mdp = msgs[i];
        safe_strcpy(buf, mdp->message.c_str());
        if (!translatable) {
            strip_translation(buf);
        }
        fout.printf(
            "<msg>\n"
            " <project>%s</project>\n"
            " <pri>%d</pri>\n"
            " <seqno>%d</seqno>\n"
            " <body><![CDATA[\n%s\n]]></body>\n"
            " <time>%d</time>\n",
            mdp->project_name,
            mdp->priority,
            mdp->seqno,
            buf,
            mdp->timestamp
        );
        fout.printf("</msg>\n");
    }
    fout.printf("</msgs>\n");
}

int MESSAGE_DESCS::highest_seqno() {
    if (msgs.size()) return msgs[0]->seqno;
    return 0;
}

void MESSAGE_DESCS::cleanup() {
    for (unsigned int i=0; i<msgs.size(); i++) {
        delete msgs[i];
    }
    msgs.clear();
}

string app_list_string(PROJECT* p) {
    string app_list;
    for (unsigned int i=0; i<gstate.apps.size(); i++) {
        APP* app = gstate.apps[i];
        if (app->project != p) continue;
        if (!app_list.empty()) {
            app_list += ", ";
        }
        app_list += "'";
        app_list += app->name;
        app_list += "'";
    }
    if (app_list.empty()) {
        app_list = "None";
    }
    return app_list;
}
