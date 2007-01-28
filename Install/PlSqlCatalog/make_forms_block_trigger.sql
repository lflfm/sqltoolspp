PROMPT --Make Forms Block Trigger
DECLARE
  gv_pack_name 			VARCHAR2(30) NOT NULL := 'orders_trg_pack'; 
  gv_table_name 		VARCHAR2(30) NOT NULL := 'orders'; 
  gv_view_name  		VARCHAR2(30) NOT NULL := 'v_orders_plus'; 
  gv_block_name 		VARCHAR2(30) NOT NULL := 'order'; 
  gv_pk_column      VARCHAR2(30) NOT NULL := 'id'; -- or rowid
  gv_pk_view_column VARCHAR2(30) NOT NULL := 'id'; -- or rowid

  CURSOR crs_tab_columns (p_table_name VARCHAR2) IS
    SELECT Lower(column_name) column_name,
       data_type||Decode(data_type,'NUMBER',Decode(Nvl(data_precision,0)+Nvl(data_scale,0),0,NULL,
       '('||data_precision||Decode(Nvl(data_scale,0),0,NULL,','||data_scale)||')'),
       'LONG',NULL,'DATE',NULL,'('||data_length||')') data_type
      FROM user_tab_columns
      WHERE table_name = UPPER(p_table_name)
      ORDER BY column_id;
  lv_start BOOLEAN;
  
  PROCEDURE put_tab_cols (p_prefix VARCHAR2 := NULL, p_datetype BOOLEAN := FALSE, 
                          p_indent PLS_INTEGER := 6, p_table_name VARCHAR2 := gv_table_name,
                          p_two_col BOOLEAN := FALSE, p_prefix2 VARCHAR2 := NULL) IS
  BEGIN
    lv_start := TRUE;
    FOR buf_column IN crs_tab_columns(p_table_name) LOOP
      FOR i IN 1..p_indent LOOP
        Dbms_Output.Put(' ');
      END LOOP;
      IF lv_start THEN
        lv_start := FALSE;
      ELSIF NOT p_datetype THEN
        Dbms_Output.Put(',');
      END IF; 
      Dbms_Output.Put(p_prefix||buf_column.column_name);
      IF p_two_col THEN
        Dbms_Output.Put(p_prefix2||buf_column.column_name);
      ELSIF p_datetype THEN
        Dbms_Output.Put(' '||buf_column.data_type||';');
      END IF; 
      Dbms_Output.New_Line;
    END LOOP;
  END;

  PROCEDURE put_blk_cols(p_indent PLS_INTEGER := 6)  IS
  BEGIN
    put_tab_cols(':'||gv_block_name||'.', FALSE, p_indent);
  END;
  
  PROCEDURE put_var_cols(p_indent PLS_INTEGER := 4)  IS
  BEGIN
    put_tab_cols('lv_',TRUE, p_indent);
  END;
  
  PROCEDURE put_blk_cols_ex (p_indent PLS_INTEGER := 6)  IS
  BEGIN
    put_tab_cols(':'||gv_block_name||'.', FALSE, p_indent, p_table_name=>gv_view_name);
  END;
  
  PROCEDURE put_tab_cols_ex (p_indent PLS_INTEGER := 6)  IS
  BEGIN
    put_tab_cols(p_indent=>p_indent, p_table_name=>gv_view_name);
  END;
  
BEGIN
-------------------------------------------------------------------------------
  Dbms_Output.Put_Line('---------------------------------------------------------------');
  Dbms_Output.Put_Line('-- FORMS BLOCK TRIGGER GENERATOR v1.00 Koch 13.02.98 00:28:28');
  Dbms_Output.Put_Line('-- GENERATED '||SYSDATE);
  Dbms_Output.Put_Line('--   Package    : '||gv_pack_name);
  Dbms_Output.Put_Line('--   Base Block : '||gv_block_name);
  Dbms_Output.Put_Line('--   Join View  : '||gv_view_name);
  Dbms_Output.Put_Line('--   Base Table : '||gv_table_name);
  Dbms_Output.Put_Line('--     Join condition : '||gv_table_name||'.'||gv_pk_column||' = '||gv_view_name||'.'||gv_pk_view_column);
  Dbms_Output.Put_Line('---------------------------------------------------------------');
  Dbms_Output.Put_Line('PACKAGE '||gv_pack_name||' IS');
    Dbms_Output.Put_Line('  PROCEDURE on_insert;');
    Dbms_Output.Put_Line('  PROCEDURE on_lock;');
    Dbms_Output.Put_Line('  PROCEDURE on_update;');
    Dbms_Output.Put_Line('  PROCEDURE on_delete;');
    Dbms_Output.Put_Line('  PROCEDURE ut_record_reread;');
  Dbms_Output.Put_Line('END '||gv_pack_name||';');
  Dbms_Output.Put_Line('---------------------------------------------------------------');
  Dbms_Output.Put_Line('-- USED PARAMETERS:');
  Dbms_Output.Put_Line('--  gv_pack_name      VARCHAR2(30) NOT NULL := ''orders_trg_pack'';'); 
  Dbms_Output.Put_Line('--  gv_table_name     VARCHAR2(30) NOT NULL := ''orders'';');
  Dbms_Output.Put_Line('--  gv_view_name      VARCHAR2(30) NOT NULL := ''v_orders_plus'';');
  Dbms_Output.Put_Line('--  gv_block_name     VARCHAR2(30) NOT NULL := ''order'';');
  Dbms_Output.Put_Line('--  gv_pk_column      VARCHAR2(30) NOT NULL := ''id'';');
  Dbms_Output.Put_Line('--  gv_pk_view_column VARCHAR2(30) NOT NULL := ''id'';');
  Dbms_Output.Put_Line('---------------------------------------------------------------');
  -------------------------------------------------------------------------------
  -------------------------------------------------------------------------------
  Dbms_Output.New_Line;
  Dbms_Output.Put_Line('---------------------------------------------------------------');
  Dbms_Output.Put_Line('-- FORMS BLOCK TRIGGER GENERATOR v1.00 Koch 13.02.98 00:28:28');
  Dbms_Output.Put_Line('---------------------------------------------------------------');
  Dbms_Output.Put_Line('PACKAGE BODY '||gv_pack_name||' IS');
    Dbms_Output.New_Line;
    Dbms_Output.Put_Line('  PROCEDURE on_insert IS');
    Dbms_Output.Put_Line('  BEGIN');
    Dbms_Output.Put_Line('    INSERT INTO '||gv_table_name||' (');
    put_tab_cols;
    Dbms_Output.Put_Line('    ) VALUES(');
    put_tab_cols(':'||gv_block_name||'.');
    Dbms_Output.Put_Line('    );');
    Dbms_Output.Put_Line('  END on_insert;');
    Dbms_Output.New_Line;
  -------------------------------------------------------------------------------
    Dbms_Output.Put_Line('  PROCEDURE on_lock IS');
    Dbms_Output.Put_Line('    RecordLocked EXCEPTION;');
    Dbms_Output.Put_Line('    PRAGMA EXCEPTION_INIT(RecordLocked,-54);');
    put_var_cols;
    Dbms_Output.Put_Line('  BEGIN');
    Dbms_Output.Put_Line('    SELECT');
    put_tab_cols;
    Dbms_Output.Put_Line('    INTO');
    put_blk_cols;
    Dbms_Output.Put_Line('    FROM '||gv_table_name);
    Dbms_Output.Put_Line('    WHERE '||gv_pk_column||' = :'||gv_block_name||'.'||gv_pk_view_column);
    Dbms_Output.Put_Line('    FOR UPDATE OF');
    put_tab_cols;
    Dbms_Output.Put_Line('    ;');
    lv_start := TRUE;
    Dbms_Output.Put_line('    IF');
    FOR buf_column IN crs_tab_columns(gv_table_name) LOOP
      Dbms_Output.Put('      ');
      IF lv_start THEN
        lv_start := FALSE;
      ELSE
        Dbms_Output.Put('OR ');
      END IF; 
      Dbms_Output.Put_line('lv_'||buf_column.column_name||' <> :'||gv_block_name||'.'||buf_column.column_name);
    END LOOP;
    Dbms_Output.Put_Line('    THEN');
    Dbms_Output.Put_Line('      msg_need_refresh_data;');
    Dbms_Output.Put_Line('      RAISE Form_Trigger_Failure;');
    Dbms_Output.Put_Line('    END IF;');
    Dbms_Output.Put_Line('  EXCEPTION WHEN RecordLocked THEN');
    Dbms_Output.Put_Line('    msg_other_user_lock_data;');
    Dbms_Output.Put_Line('    RAISE Form_Trigger_Failure;');
    Dbms_Output.Put_Line('  END on_lock;');
    Dbms_Output.New_Line;
  -------------------------------------------------------------------------------
    Dbms_Output.Put_Line('  PROCEDURE on_update IS');
    Dbms_Output.Put_Line('  BEGIN');
    Dbms_Output.Put_Line('    UPDATE '||gv_table_name||' SET');
    put_tab_cols(p_two_col=>TRUE, p_prefix2=>' = :'||gv_block_name||'.');
    Dbms_Output.Put_Line('    WHERE '||gv_pk_column||' = :'||gv_block_name||'.'||gv_pk_view_column||';');
    Dbms_Output.Put_Line('    EXECUTE_TRIGGER(''UT_RECORD_REREAD'');');
    Dbms_Output.Put_Line('  END on_update;');
    Dbms_Output.New_Line;
  -------------------------------------------------------------------------------
    Dbms_Output.Put_Line('  PROCEDURE on_delete IS');
    Dbms_Output.Put_Line('  BEGIN');
    Dbms_Output.Put_Line('    DELETE '||gv_table_name);
    Dbms_Output.Put_Line('    WHERE '||gv_pk_column||' = :'||gv_block_name||'.'||gv_pk_view_column||';');
    Dbms_Output.Put_Line('  END on_delete;');
    Dbms_Output.New_Line;
  -------------------------------------------------------------------------------
    Dbms_Output.Put_Line('  PROCEDURE ut_record_reread IS');
    Dbms_Output.Put_Line('  BEGIN');
    Dbms_Output.Put_Line('    SELECT');
    put_tab_cols_ex;
    Dbms_Output.Put_Line('    INTO');
    put_blk_cols_ex;
    Dbms_Output.Put_Line('    FROM '||gv_view_name);
    Dbms_Output.Put_Line('    WHERE '||gv_pk_view_column||' = :'||gv_block_name||'.'||gv_pk_column||';');
    Dbms_Output.Put_Line('  END ut_record_reread;');
    Dbms_Output.New_Line;
  -------------------------------------------------------------------------------
  Dbms_Output.Put_Line('END '||gv_pack_name||';');
END;
/
