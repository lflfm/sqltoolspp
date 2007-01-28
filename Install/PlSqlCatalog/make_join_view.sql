PROMPT make view
DECLARE
  gv_view_name VARCHAR2(30); 
  --gv_where_clause VARCHAR2(256); 

  TYPE TTabInfoRow IS RECORD (
    table_name  VARCHAR2(30),
    table_alias VARCHAR2(30),
    col_prefix  VARCHAR2(30),
    join_clause VARCHAR2(128)
  );
  
  TYPE TTabInfo IS TABLE
    OF TTabInfoRow
    INDEX BY BINARY_INTEGER;
    
  TabInfo TTabInfo;
  
  CURSOR crs_tab_columns (p_table_name VARCHAR2) IS
    SELECT Lower(column_name) column_name
      FROM user_tab_columns
      WHERE table_name = Upper(p_table_name);
      
  lv_start BOOLEAN;
  lv_index PLS_INTEGER := 1;
    
  PROCEDURE add_table (p_name VARCHAR2, p_alias VARCHAR2, p_prefix VARCHAR2, p_join_clause VARCHAR2) IS
  BEGIN
    TabInfo(lv_index).table_name  := Lower(p_name);
    TabInfo(lv_index).table_alias := Lower(p_alias);
    TabInfo(lv_index).col_prefix  := Lower(p_prefix);
    TabInfo(lv_index).join_clause := Lower(p_join_clause);
    lv_index := lv_index + 1;
  END;
  
BEGIN
-- Initializations
  gv_view_name := 'v_expanded_orders';
  add_table('orders',         'o',    NULL,              NULL);
  add_table('wagons',         'w',    'wagon',           'o.wagon_code = w.code');
  add_table('clients',        'pc',   'parent_client',   'o.parent_client_id = pc.id(+)');
  add_table('clients',        'cc',   'child_client',    'o.child_client_id = cc.id(+)');
  add_table('stations',       'dps',  'depart_station',  'o.departure_station_code = dps.code(+)');
  add_table('railways',       'dpr',  'depart_railway',  'dps.railway_code = dpr.code(+)');
  add_table('stations',       'dss',  'dest_station',    'o.destination_station_code = dss.code(+)');
  add_table('railways',       'dsr',  'dest_railway',    'dss.railway_code = dsr.code(+)');
  add_table('loades',         'l',    'load',            'o.load_code = l.code(+)');
  add_table('services',       's',    'service',         'o.service_id = s.id');
  add_table('v_current_dislocations', 'd',    'disl',    'o.wagon_code = d.wagon_code(+)');
  add_table('v_s251_short',   's251', 's251',            'o.s251_id = s251.id(+)');
  --gv_where_clause := 'o.wagon_code = d.wagon_code'; 
  
-- Initializations

  Dbms_Output.Put_Line('CREATE OR REPLACE');
  Dbms_Output.Put_Line('VIEW '||Lower(gv_view_name)||' AS');
  
  Dbms_Output.Put_Line('SELECT');
  lv_start := TRUE;
  FOR i IN 1..TabInfo.COUNT LOOP
    FOR buf_column IN crs_tab_columns(TabInfo(i).table_name) LOOP
      Dbms_Output.Put('  ');
      IF lv_start THEN
        lv_start := FALSE;
      ELSE
        Dbms_Output.Put(',');
      END IF;
      IF TabInfo(i).table_alias IS NOT NULL THEN
        Dbms_Output.Put(TabInfo(i).table_alias||'.');
      END IF;
      Dbms_Output.Put(buf_column.column_name);
      IF TabInfo(i).col_prefix IS NOT NULL THEN
        Dbms_Output.Put(' '||TabInfo(i).col_prefix||'#'||buf_column.column_name);
      END IF;
      Dbms_Output.New_Line;
    END LOOP;
  END LOOP;
  
  Dbms_Output.Put_Line('FROM');
  lv_start := TRUE;
  FOR i IN 1..TabInfo.COUNT LOOP
    Dbms_Output.Put('  ');
    IF lv_start THEN
      lv_start := FALSE;
    ELSE
      Dbms_Output.Put(',');
    END IF;
    Dbms_Output.Put(TabInfo(i).table_name);
    IF TabInfo(i).table_alias IS NOT NULL THEN
      Dbms_Output.Put(' '||TabInfo(i).table_alias);
    END IF;
    Dbms_Output.New_Line;
  END LOOP;
  
  Dbms_Output.Put_Line('WHERE');
  lv_start := TRUE;
  FOR i IN 1..TabInfo.COUNT LOOP
    IF TabInfo(i).join_clause IS NOT NULL THEN
      Dbms_Output.Put('  ');
      IF lv_start THEN
        lv_start := FALSE;
      ELSE
        Dbms_Output.Put('AND ');
      END IF;
      Dbms_Output.Put_Line(TabInfo(i).join_clause);
    END IF;
  END LOOP;
  
  Dbms_Output.Put_Line('WITH READ ONLY;');

END;
/
