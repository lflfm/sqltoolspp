connect sys

PROMPT 
PROMPT Библиотечный кеш (если < 1 , то Ok)

SELECT SUM(pins)    "Executions",
       SUM(reloads) "Cache misses",
       SUM(reloads)/SUM(pins)*100 "Persent misses"
  FROM v$librarycache;

PROMPT 
PROMPT Кеш словаря данных (если < 10 , то Ok)

SELECT SUM(gets)       "Data dictionary gets",
       SUM(getmisses) "Get misses",
       SUM(getmisses)/SUM(gets)*100 "Persent misses"
  FROM v$rowcache;

PROMPT 
PROMPT Кеш данных (если < 60 , то BAD)
variable hit NUMBER;
execute begin :hit := 100 * db_buffer_hit_ratio; end;
print hit;


PROMPT 
PROMPT Прогноз на увеличение кеша

COLUMN "Interval"          FORMAT A30

SELECT 250*TRUNC(indx/250) + 1 || ' to ' || 250*(TRUNC(indx/250)+1) "Interval",
       SUM(count) "Buffer Cache Hits"
       FROM sys.x$kcbrbh
       WHERE indx > 0
       GROUP BY TRUNC(indx/250);

PROMPT 
PROMPT Прогноз на уменьшение кеша

COLUMN "Interval"          FORMAT A30

SELECT 250*TRUNC(indx/250) + 1 || ' to ' || 250*(TRUNC(indx/250)+1) "Interval",
       SUM(count) "Buffer Cache Hits"
       FROM sys.x$kcbcbh
       WHERE indx > 0
       GROUP BY TRUNC(indx/250);

SELECT count "Buffer Cache Hits Total"
       FROM sys.x$kcbcbh
       WHERE indx = 0;
