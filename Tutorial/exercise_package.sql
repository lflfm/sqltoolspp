CREATE OR REPLACE PACKAGE tmk IS
  PROCEDURE test;
END tmk;
/
CREATE OR REPLACE PACKAGE BODY tmk IS
  PROCEDURE test IS
    rec_tmp atmk%ROWTYPE;
  BEGIN
    SELECT * INTO rec_tmp FROM atmk WHERE ROWNUM=1;
    Dbms_Output.Put_Line(rec_tmp.table_name || ',' ||
    rec_tmp.tablespace_name || ',' || rec_tmp.cluster_name || '.');
  END test;
END tmk;
/
BEGIN
  tmk.test;
END;
/