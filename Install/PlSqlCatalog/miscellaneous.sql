REM ---------------------------------------------------------------------------
REM --
REM -- Contents:
REM --   1)  Move all index in "USER_INDEX"
REM --   2)  Drop all user objects
REM --   3)  Show user extenst fragmentatuion
REM --   4)  Show user extenst fragmentatuion (for DBA)
REM --
REM ---------------------------------------------------------------------------

REM -- 1)  Move all index in "USER_INDEX"
SELECT
  'alter index '||OWNER||'.'||INDEX_NAME||' rebuild tablespace user_index;'
FROM all_indexes
WHERE owner NOT IN ('SYS','SYSTEM','DES2000')
  AND tablespace_name <> 'USER_INDEX'
/

REM -- 2)  Drop all user objects
SELECT
 'drop '||lower(object_type)||' '||owner||'.'||object_name||Decode(object_type,'TABLE',' cascade constraint;',';')
FROM all_objects
WHERE owner = 'APP$WAGE'
  AND object_type NOT IN ('INDEX','TRIGGER','PACKAGE BODY')
/

REM -- 3)  Show user extenst fragmentatuion
SELECT segment_name,Count(*),Sum(bytes)/(1024*1024),Sum(blocks) FROM user_extents
  GROUP BY segment_name
  ORDER BY 2 DESC, 3 DESC
/

REM -- 4)  Show user extenst fragmentatuion (for DBA)
SELECT segment_name,Count(*),Sum(bytes)/(1024*1024),Sum(blocks) FROM dba_extents
  WHERE owner = 'SYS'
  GROUP BY segment_name
  ORDER BY 2 DESC, 3 DESC
/