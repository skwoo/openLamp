DROP PROCEDURE sp_mSync_VerCopy
GO
CREATE PROCEDURE sp_mSync_VerCopy(
		@m_strAppName varchar(30),
		@m_nVersion INT,
		@m_nNewVersion INT
)
as
declare @m_nNewVersionID INT
declare @m_nVersionID INT
declare @nDBID INT, @nNewDBID INT, @nTableID INT, @nNewTableID INT

declare @strDSNName varchar(30), @strDSNUid  varchar(30), @strDSNPwd  varchar(30), @strCliDSN  varchar(30)
declare @strSvrTableName varchar(30), @strCliTableName varchar(30)
declare @strEvent varchar(1), @strScript varchar(4000)

begin
	begin transaction

	-- 기존 application의 versionID를 구한다
	IF NOT EXISTS 
	(SELECT versionID FROM openMSYNC_version
	WHERE application = @m_strAppName AND version = @m_nVersion)
	BEGIN
		PRINT @m_strAppName+'의 버전 (' + str(@m_nVersion) + ')의 정보를 찾을 수 없습니다'
		GOTO sp_END
	END

	IF (@m_nNewVersion <=0 OR @m_nNewVersion >50)
	BEGIN
		PRINT 'Application의 version은 1과 50 사이의 정수입니다.'
		GOTO sp_END
	END

	if @@ERROR <> 0	 GOTO sp_END

	SELECT @m_nVersionID = versionID FROM openMSYNC_version
	WHERE application = @m_strAppName AND version = @m_nVersion
	-- max version ID를 구한다 ==> m_nNewVersionID
	SELECT @m_nNewVersionID=max(versionID)+1 FROM openMSYNC_version  
	if @@ERROR <> 0	GOTO sp_END

	-- m_nNewVersionID로 새 version을 insert
	INSERT INTO openMSYNC_version VALUES(@m_nNewVersionID, @m_strAppName, @m_nNewVersion)	

	if @@ERROR <> 0	GOTO sp_END

	-- 기존 application의 DB, DSN 정보를 fetch
	DECLARE DB_cursor CURSOR FOR
	SELECT DBID, svrDSN, DSNUserID, DSNPwd, cliDSN 
	FROM openMSYNC_DSN 
	WHERE VersionID = @m_nVersionID ORDER BY svrDSN

	if @@ERROR <> 0	GOTO sp_END

	OPEN DB_cursor

	if @@ERROR <> 0	GOTO sp_END
	
	FETCH NEXT FROM DB_cursor
	INTO @nDBID, @strDSNName, @strDSNUid, @strDSNPwd, @strCliDSN

	if @@ERROR <> 0	GOTO sp_DB_END

	WHILE @@FETCH_STATUS = 0
	BEGIN
		-- max DBID를 구한다 ==> nNewDBID
		SELECT @nNewDBID = max(DBID)+1 FROM openMSYNC_dsn 
		if @@ERROR <> 0	GOTO sp_DB_END

		-- nNewDBID로 새 DSN을 insert
		INSERT INTO openMSYNC_DSN VALUES(@nNewDBID, @m_nNewVersionID, @strDSNName, @strDSNUid, @strDSNPwd, @strCliDSN)
		if @@ERROR <> 0	GOTO sp_DB_END

		-- 기존 DSN의 tableID, tableName 정보를 fetch
		DECLARE TABLE_cursor CURSOR FOR
		SELECT TableID, SvrTableName, CliTableName 
		FROM openMSYNC_table 
		WHERE DBID = @nDBID ORDER BY SvrTableName
		if @@ERROR <> 0	GOTO sp_DB_END
  
		OPEN TABLE_cursor

		FETCH NEXT FROM TABLE_cursor
		INTO @nTableID, @strSvrTableName, @strCliTableName
		if @@ERROR <> 0	GOTO sp_DB_TABLE_END

		WHILE @@FETCH_STATUS = 0
		BEGIN
			-- max TableID를 구한다 ==> nNewTableID
			SELECT @nNewTableID = max(TableID)+1 FROM openMSYNC_table  
			if @@ERROR <> 0	GOTO sp_DB_TABLE_END

			-- nNewTableID로 새 table을 insert
			INSERT INTO openMSYNC_table VALUES(@nNewTableID, @nNewDBID, @strSvrTableName, @strCliTableName)
			if @@ERROR <> 0	GOTO sp_DB_TABLE_END

			-- 기존 table의 script 정보를 fetch
			DECLARE SCRIPT_cursor CURSOR FOR
			SELECT Event, Script 
			FROM openMSYNC_script 
			WHERE TableID = @nTableID
			if @@ERROR <> 0	GOTO sp_DB_TABLE_END

			OPEN SCRIPT_cursor
			if @@ERROR <> 0	GOTO sp_DB_TABLE_END

			FETCH NEXT FROM SCRIPT_cursor
			INTO @strEvent, @strScript
			if @@ERROR <> 0	GOTO sp_DB_TABLE_SCRIPT_END

			WHILE @@FETCH_STATUS = 0
			BEGIN
				-- 새 script 정보를 insert
				INSERT INTO openMSYNC_script VALUES(@nNewTableID, @strEvent, @strScript)
				if @@ERROR <> 0	GOTO sp_DB_TABLE_SCRIPT_END
				FETCH NEXT FROM SCRIPT_cursor
				INTO @strEvent, @strScript				
				if @@ERROR <> 0	GOTO sp_DB_TABLE_SCRIPT_END
			END
			CLOSE SCRIPT_cursor
			DEALLOCATE SCRIPT_cursor

			FETCH NEXT FROM TABLE_cursor
			INTO @nTableID, @strSvrTableName, @strCliTableName
			if @@ERROR <> 0	GOTO sp_DB_TABLE_END
		END
		CLOSE TABLE_cursor
		DEALLOCATE TABLE_cursor

		FETCH NEXT FROM DB_cursor
		INTO @nDBID, @strDSNName, @strDSNUid, @strDSNPwd, @strCliDSN
		if @@ERROR <> 0	GOTO sp_DB_END
	END

	CLOSE DB_cursor
	DEALLOCATE DB_cursor
	commit;
	goto MAIN_END

sp_DB_TABLE_SCRIPT_END:
	PRINT '복사 중에 에러가 발생하였습니다. : script 정보 복사'
	CLOSE SCRIPT_cursor
	DEALLOCATE SCRIPT_cursor
sp_DB_TABLE_END:
	PRINT '복사 중에 에러가 발생하였습니다. : table 정보 복사'
	CLOSE TABLE_cursor
	DEALLOCATE TABLE_cursor
sp_DB_END:
	PRINT '복사 중에 에러가 발생하였습니다.'
	CLOSE DB_cursor
	DEALLOCATE DB_cursor
sp_END:
	rollback
MAIN_END:
end

