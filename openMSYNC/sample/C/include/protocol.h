#ifndef _MSYNC_PROTOCOL_H_
#define _MSYNC_PROTOCOL_H_

/* ***************************************************************************
 ***  format:  OP_CODE(2) | BODY_LENGTH(sizeof(u_int)) | BODY(~)	 
 *************************************************************************** */
#define PACKET_OP_CODE_SIZE        (2)
#define PACKET_BUFFER_SIZE      (8192)
#define PACKET_BODY_LENGTH_SIZE (sizeof(unsigned int))
#define PACKET_BODY_SIZE        (PACKET_BUFFER_SIZE - PACKET_BODY_LENGTH_SIZE)


#define UPDATE_FLAG    'U'      // Update
#define DELETE_FLAG    'D'      // Delete
#define INSERT_FLAG    'I'      // Insert
#define DOWNLOAD_FLAG  'F'      //Fetch
#define DOWNLOAD_DELETE_FLAG 'R'        // Remove
#define END_FLAG	   'E'  // End
#define OK_FLAG        'K'      // OK
#define TABLE_FLAG     'M'      // more tables
#define APPLICATION_FLAG 'S'    // DBSYNC or FILESYNC
#define ACK_FLAG	   'A'  // ACK
#define NACK_FLAG	   'N'  // Nack
#define XACK_FLAG	   'X'  // Excess
#define UPGRADE_FLAG   'V'
#define	END_OF_TRANSACTION 'T'  // EOT
#define NO_SCRIPT_FLAG 'B'      /* No Script */


#define ALL_SYNC 1
#define MOD_SYNC 2

#define DBSYNC       (1)


#define FIELD_DEL 0x08
#define RECORD_DEL 0x7F

#define HUGE_STRING_LENGTH	8156
//#define CIPHER_BLOCK_LENGTH 32

#define NORMAL 1
#define ABNORMAL 2


#endif //_MSYNC_PROTOCOL_H_
