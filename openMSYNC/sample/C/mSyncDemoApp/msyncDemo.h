#ifndef _MSYNC_DEMO_H_
#define _MSYNC_DEMO_H_
typedef struct
{
    int ID;
    char username[32];
    char Company[256];
    char Name[256];
    char e_mail[256];
    char Company_phone[21];
    char Mobile_phone[21];
    char Address[256];
    char mTime[64];
#ifndef USE_MSYNC_TABLE
    int dflag;
    /*
       0: 아무런 작업이 없음.

       1: local에서 insert 됨.
       2: local에서 update 됨.
       3: local에서 delete 됨.

       0: server에서 가져와서 insert 함.
       0: server에서 가져와서 update 함.
       6: server에서 delete 되었음을 인지함(?)
     */
#define DFLAG_INSERTED (1)
#define DFLAG_UPDATED  (2)
#define DFLAG_DELETED  (3)
#define DFLAG_SYNCED   (4)
#endif
} S_Recoed;





#endif // _MSYNC_DEMO_H_
