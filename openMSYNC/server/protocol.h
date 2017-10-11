/* 
    This file is part of openMSync Server module, DB synchronization software.

    Copyright (C) 2012 Inervit Co., Ltd.
        support@inervit.com

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

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

#define NORMAL 1
#define ABNORMAL 2


#endif //_MSYNC_PROTOCOL_H_
