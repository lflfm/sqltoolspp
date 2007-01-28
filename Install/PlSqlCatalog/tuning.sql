connect sys

PROMPT 
PROMPT ������������ ��� (���� < 1 , �� Ok)

SELECT SUM(pins)    "Executions",
       SUM(reloads) "Cache misses",
       SUM(reloads)/SUM(pins)*100 "Persent misses"
  FROM v$librarycache;

PROMPT 
PROMPT ��� ������� ������ (���� < 10 , �� Ok)

SELECT SUM(gets)       "Data dictionary gets",
       SUM(getmisses) "Get misses",
       SUM(getmisses)/SUM(gets)*100 "Persent misses"
  FROM v$rowcache;

PROMPT 
PROMPT ��� ������ (���� < 60 , �� BAD)
variable hit NUMBER;
execute begin :hit := 100 * db_buffer_hit_ratio; end;
print hit;


PROMPT 
PROMPT ������� �� ���������� ����

COLUMN "Interval"          FORMAT A30

SELECT 250*TRUNC(indx/250) + 1 || ' to ' || 250*(TRUNC(indx/250)+1) "Interval",
       SUM(count) "Buffer Cache Hits"
       FROM sys.x$kcbrbh
       WHERE indx > 0
       GROUP BY TRUNC(indx/250);

PROMPT 
PROMPT ������� �� ���������� ����

COLUMN "Interval"          FORMAT A30

SELECT 250*TRUNC(indx/250) + 1 || ' to ' || 250*(TRUNC(indx/250)+1) "Interval",
       SUM(count) "Buffer Cache Hits"
       FROM sys.x$kcbcbh
       WHERE indx > 0
       GROUP BY TRUNC(indx/250);

SELECT count "Buffer Cache Hits Total"
       FROM sys.x$kcbcbh
       WHERE indx = 0;
