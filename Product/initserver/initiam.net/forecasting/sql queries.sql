SELECT	AVG(gbchange / tmchange) AS DeltaGBperSec INTO @DeltaGBperSec
FROM	(
	SELECT	s.EntryDate AS x,
			UNIX_TIMESTAMP(s.EntryDate) AS UnixTime,
			(s.DriveSpace/(1024*1024*1024)) AS y,
			if(@last_entrygb = 0, 0, (s.DriveSpace/(1024*1024*1024)) - @last_entrygb) AS gbchange,
			if(@last_entrytm = 0, 0, UNIX_TIMESTAMP(s.EntryDate) - @last_entrytm) AS tmchange,
			@last_entrygb := (s.DriveSpace/(1024*1024*1024)),
			@last_entrytm := UNIX_TIMESTAMP(s.EntryDate)
	FROM	(
		select 	@last_entrygb := 0,
				@last_entrytm := 0
	) z,
			(			
		SELECT	Name,
				DriveSpace,
				DriveCapacity,
				EntryDate
		FROM	ComputerInformation
		WHERE	Name LIKE 'INITIAMCLIENT'
		AND		EntryDate BETWEEN CURDATE() - INTERVAL 30 DAY AND CURDATE()
		ORDER BY EntryDate DESC
	) s
	ORDER BY EntryDate ASC
) a;

SELECT @DeltaGBperSec;

SELECT	s.EntryDate AS x,
		UNIX_TIMESTAMP(s.EntryDate) AS UnixTime,
		(s.DriveSpace/(1024*1024*1024)) AS y,
		if(@last_entrygb = 0, 0, (s.DriveSpace/(1024*1024*1024)) - @last_entrygb) AS gbchange,
		if(@last_entrytm = 0, 0, UNIX_TIMESTAMP(s.EntryDate) - @last_entrytm) AS tmchange,
		@last_entrygb := (s.DriveSpace/(1024*1024*1024)),
		@last_entrytm := UNIX_TIMESTAMP(s.EntryDate)
FROM	(
	select 	@last_entrygb := 0,
			@last_entrytm := 0
) z,
		(			
	SELECT	Name,
			DriveSpace,
			DriveCapacity,
			EntryDate
	FROM	ComputerInformation
	WHERE	Name LIKE 'INITIAMCLIENT'
	AND		EntryDate BETWEEN CURDATE() - INTERVAL 30 DAY AND CURDATE()
	ORDER BY EntryDate DESC
) s
ORDER BY EntryDate ASC;

SELECT	Name,
		DriveSpace,
		DriveCapacity,
		EntryDate,
		@DeltaGBperSec AS DeltaGBperSec,
		DriveSpace / @DeltaGBperSec AS SecTilFull,
		2147483647 AS LastUnixTime,
		CASE
			WHEN (UNIX_TIMESTAMP(EntryDate) + (DriveSpace / @DeltaGBperSec)) < 2147483647 THEN FROM_UNIXTIME((UNIX_TIMESTAMP(EntryDate) + (DriveSpace / @DeltaGBperSec)))
			ELSE FROM_UNIXTIME(2147483647)#STR_TO_DATE('2038-01-19 03:14:07', '%Y-%m-%d %H:%i:%s')
		END AS DateWhenFull,
		#FROM_UNIXTIME((UNIX_TIMESTAMP(EntryDate) + (DriveSpace / @DeltaGBperSec))) AS DateWhenFull,
		((DriveSpace / @DeltaGBperSec) DIV 31556926) AS Years,
		(((DriveSpace / @DeltaGBperSec) % 31556926) DIV 2629743) AS Months,
		((((DriveSpace / @DeltaGBperSec) % 31556926) % 2629743) DIV 604800) AS Weeks,
		(((((DriveSpace / @DeltaGBperSec) % 31556926) % 2629743) % 604800) DIV 86400) AS Days,
		((((((DriveSpace / @DeltaGBperSec) % 31556926) % 2629743) % 604800) % 86400) DIV 3600) AS Hours,
		(((((((DriveSpace / @DeltaGBperSec) % 31556926) % 2629743) % 604800) % 86400) % 3600) DIV 60) AS Minutes,
		((((((((DriveSpace / @DeltaGBperSec) % 31556926) % 2629743) % 604800) % 86400) % 3600) % 60) DIV 1) AS Seconds
INTO	@name,
		@drivespace,
		@drivecapacity,
		@entrydate,
		@DeltaGBperSec,
		@sectilfull,
		@lastunixtime,
		@datewhenfull,
		@years,
		@months,
		@weeks,
		@days,
		@hours,
		@minutes,
		@seconds
FROM	ComputerInformation
WHERE	Name LIKE 'INITIAMCLIENT'
AND		EntryDate BETWEEN CURDATE() - INTERVAL 30 DAY AND CURDATE()
ORDER BY EntryDate DESC LIMIT 1;

SELECT @name,
		@drivespace,
		@entrydate,
		@DeltaGBperSec,
		@sectilfull,
		@lastunixtime,
		@datewhenfull,
		@years,
		@months,
		@weeks,
		@days,
		@hours,
		@minutes,
		@seconds;

(
SELECT	UNIX_TIMESTAMP(s.EntryDate) AS x,
		(s.DriveSpace/(1024*1024*1024)) AS y
FROM	(			
	SELECT	Name,
			DriveSpace,
			DriveCapacity,
			EntryDate
	FROM	ComputerInformation
	WHERE	Name LIKE 'INITIAMCLIENT'
	AND		EntryDate BETWEEN CURDATE() - INTERVAL 30 DAY AND CURDATE()
	ORDER BY EntryDate DESC
) s
ORDER BY EntryDate ASC
)
UNION ALL
(
SELECT	IF((UNIX_TIMESTAMP(@entrydate) + @sectilfull) <= @lastunixtime, (UNIX_TIMESTAMP(@entrydate) + @sectilfull) - 60, @lastunixtime - 60) AS x,
		NULL AS y
)
UNION ALL
(
SELECT	IF((UNIX_TIMESTAMP(@entrydate) + @sectilfull) <= @lastunixtime, (UNIX_TIMESTAMP(@entrydate) + @sectilfull), @lastunixtime) AS x,
		0 AS y
);




#Input:
#	Name (SELECT Name FROM ComputerInformation GROUP BY Name, ComputerGUID)
#	%ChangeUsage (default 0)