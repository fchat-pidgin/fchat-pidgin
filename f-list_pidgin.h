#ifndef F_LIST_PIDGIN_H
#define	F_LIST_PIDGIN_H

#include "f-list.h"

#define FLIST_CONV_ACCOUNT_BUTTON   "flist-account-button"
#define FLIST_CONV_ALERT_STAFF_BUTTON "flist-alert-staff-button"

void flist_pidgin_init();
void flist_pidgin_enable_signals(FListAccount *fla);
void flist_pidgin_disable_signals(FListAccount *fla);

#endif	/* F_LIST_PIDGIN_H */
