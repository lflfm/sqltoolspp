var str VARCHAR2(10)

BEGIN :str:= 'hello'; END;
/

print str


var vlob CLOB

BEGIN
  --
  :vlob := 'hello 1';
END;
/

BEGIN
  SELECT 'hello 2' INTO :vlob FROM dual;
END;
/

print vlob

var n NUMBER

BEGIN :n := 9999; END;
/

print n