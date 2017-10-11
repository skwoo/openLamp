DROP TABLE openMSYNC_Version;
DROP TABLE openMSYNC_DSN;
DROP TABLE openMSYNC_Table;
DROP TABLE openMSYNC_Script;
DROP TABLE openMSYNC_User;

CREATE TABLE openMSYNC_Version (
    VersionID       INT NOT NULL,
    Application     VARCHAR (30) NOT NULL,
    Version         INT NOT NULL,
    PRIMARY KEY(Application, Version)
);

CREATE TABLE openMSYNC_DSN(
    DBID            INT NOT NULL,
    VersionID       INT NOT NULL,
    SvrDSN          VARCHAR (30) NOT NULL,
    DSNUserID       VARCHAR (30) NOT NULL,
    DSNPwd          VARCHAR (30) NOT NULL,
    CliDSN          VARCHAR (30) NOT NULL,
    PRIMARY KEY(VersionID, CliDSN)
);

CREATE UNIQUE INDEX openMSYNC_DSN_idx ON openMSYNC_DSN(VersionID, SvrDSN);

CREATE TABLE openMSYNC_Table (
    TableID         INT NOT NULL,
    DBID            INT NOT NULL,
    SvrTableName    VARCHAR (30) NOT NULL,
    CliTableName    VARCHAR (30) NOT NULL,
    PRIMARY KEY(DBID, CliTableName)
);

CREATE UNIQUE INDEX openMSYNC_Table_idx ON openMSYNC_Table(DBID, SvrTableName);

CREATE TABLE openMSYNC_Script (
    TableID         INT NOT NULL,
    Event           CHAR (1) NOT NULL,
    Script          MEMO,
    PRIMARY KEY(TableID, Event)
);

CREATE TABLE openMSYNC_User (
    UserID          VARCHAR (20) NOT NULL PRIMARY KEY,
    UserName        VARCHAR (20),
    UserPwd         VARCHAR (20) NOT NULL,
    ConnectionFlag  CHAR (1),
    DBSyncTime      TIMESTAMP
);