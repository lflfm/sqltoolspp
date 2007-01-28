PROMPT ***************************************************************
PROMPT * Recompiling invalid objects...
PROMPT ***************************************************************

select substr(object_name,1,40) object, object_type type
from  user_objects
where status  =  'INVALID'
order by 1,2
/

SET TERMOUT ON
column nl newline

set timing off 
set heading off 
set feedback off 
set termout off 
set pages 0

spool compile.tmp

PROMPT PROMPT
PROMPT PROMPT Re-compiling invalid views...
PROMPT PROMPT

select 'PROMPT Compiling View '||object_name nl
,      'alter view '||object_name||' compile;' nl
from  user_objects
where object_type = 'VIEW'
and   status      =  'INVALID'
order by created
/

PROMPT PROMPT
PROMPT PROMPT Re-compiling invalid package specifications...
PROMPT PROMPT

select 'PROMPT Compiling Package  '||object_name nl
,      'alter package '||object_name||' compile package;' nl
from  user_objects
where object_type = 'PACKAGE'
and   status      =  'INVALID'
order by created
/

PROMPT PROMPT
PROMPT PROMPT Re-compiling invalid packages...
PROMPT PROMPT

select 'PROMPT Compiling Package Body '||object_name nl
,      'alter package '||object_name||' compile body;' nl
from  user_objects
where object_type = 'PACKAGE BODY'
and   status      =  'INVALID'
order by created
/

spool off

set termout on 
spool compile.lis

start compile.tmp

spool off
