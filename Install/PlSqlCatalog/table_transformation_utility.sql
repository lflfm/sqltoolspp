REM -------------------------------------------------------------------------
REM -- SCRIPT:     TABLE TRANSFORMATION UTILITY V1.01 BETA Koch            --
REM --             01.03.98 created                                        --
REM --             18.10.98 add grants processing                          --
REM -- PURPOSE:    will help to you to do any transformations of any table --
REM -- PARAMETERS: gv_table_name - taget table name                        --
REM --             gv_all_params - show all (and default) table parameters --
REM -- RESULT:     SQL Script, which you can modify and to execute         --
REM -------------------------------------------------------------------------

DECLARE
/* PARAMETERS *******************************************/
  gv_table_name VARCHAR2(30) := 'DOCUMENTS';
  gv_all_params BOOLEAN      := FALSE;
/* PARAMETERS *******************************************/
  lv_safe_table_name VARCHAR2(30);
  CURSOR cr_tab_triggers IS
    SELECT * FROM all_triggers
      WHERE table_owner = USER
      AND table_name = Upper(gv_table_name);
  CURSOR cr_user_desc IS
    SELECT * FROM user_users;
  CURSOR cr_table_desc IS
    SELECT * FROM user_tables
      WHERE table_name = Upper(gv_table_name);
  CURSOR cr_columns IS
    SELECT * FROM user_tab_columns
      WHERE table_name = Upper(gv_table_name)
      ORDER BY column_id;
  CURSOR cr_tablespace_desc (p_name VARCHAR2) IS
    SELECT * FROM user_tablespaces
      WHERE tablespace_name = Upper(p_name);
  CURSOR cr_constraints IS
    SELECT * FROM all_constraints
      WHERE (owner = USER AND table_name = Upper(gv_table_name))
      OR (r_owner, r_constraint_name) IN
        (SELECT owner, constraint_name FROM user_constraints
          WHERE table_name = Upper(gv_table_name))
      ORDER BY DECODE(table_name,Upper(gv_table_name),' ',table_name), constraint_type, constraint_name;
  CURSOR cr_cons_columns (p_owner VARCHAR2, p_name VARCHAR2) IS
    SELECT * FROM all_cons_columns
    WHERE constraint_name = Upper(p_name)
    AND owner = p_owner
    ORDER BY position;
  CURSOR cr_ref_constraints (p_owner VARCHAR2, p_name VARCHAR2) IS
    SELECT * FROM all_constraints
    WHERE constraint_name = Upper(p_name)
    AND owner = Upper(p_owner);
  CURSOR cr_constr_indexes (p_name VARCHAR2) IS
    SELECT * FROM user_indexes
    WHERE table_owner = USER
    AND table_name = Upper(gv_table_name)
    AND index_name = p_name;
  CURSOR cr_indexes IS
    SELECT * FROM user_indexes
    WHERE table_owner = USER
    AND table_name = Upper(gv_table_name)
    AND index_name NOT IN
      (SELECT constraint_name FROM user_constraints
         WHERE constraint_type IN ('U','P')
         AND owner = USER);
  CURSOR cr_ind_columns (p_name VARCHAR2) IS
    SELECT * FROM user_ind_columns
    WHERE table_name = Upper(gv_table_name)
       AND index_name = Upper(p_name)
       ORDER BY column_position;
  CURSOR cr_tab_comments IS
    SELECT * FROM user_tab_comments
    WHERE table_name = Upper(gv_table_name);
  CURSOR cr_col_comments IS
    SELECT * FROM user_col_comments
    WHERE table_name = Upper(gv_table_name);
  CURSOR cr_tab_grants IS
    SELECT * FROM user_tab_privs
      WHERE owner = USER
        AND table_name = Upper(gv_table_name);
  CURSOR cr_col_grants IS
    SELECT * FROM user_col_privs
      WHERE owner = USER
        AND table_name = Upper(gv_table_name);
  lv_start BOOLEAN;
  lv_counter INTEGER;
/******************************************************************/
  TYPE TAllStorageParam IS RECORD (
    pct_free        PLS_INTEGER,
    pct_used        PLS_INTEGER,
    ini_trans       PLS_INTEGER,
    max_trans       PLS_INTEGER,
    tablespace_name VARCHAR2(30),
    initial_extent  PLS_INTEGER,
    next_extent     PLS_INTEGER,
    min_extents     PLS_INTEGER,
    max_extents     PLS_INTEGER,
    pct_increase    PLS_INTEGER,
    freelists       PLS_INTEGER,
    freelist_groups PLS_INTEGER
  );
  lv_storage_param TAllStorageParam;
  lv_storage_clause BOOLEAN := FALSE;
  lv_using_index_clause BOOLEAN := FALSE;
  PROCEDURE open_storage_clause IS
  BEGIN
    IF NOT lv_storage_clause THEN
      lv_storage_clause := TRUE;
      Dbms_Output.Put_Line('  STORAGE (');
    END IF;
  END;
  PROCEDURE close_storage_clause IS
  BEGIN
    IF lv_storage_clause THEN
      Dbms_Output.Put_Line('  )');
      lv_storage_clause := FALSE;
    END IF;
  END;
  PROCEDURE open_using_index_clause (p_is_constr BOOLEAN) IS
  BEGIN
    IF p_is_constr AND NOT lv_using_index_clause THEN
      lv_using_index_clause := TRUE;
      Dbms_Output.Put_Line('  USING INDEX');
    END IF;
  END;
  PROCEDURE close_using_index_clause IS
  BEGIN
    IF lv_using_index_clause THEN
      lv_using_index_clause := FALSE;
    END IF;
  END;
  PROCEDURE print_storage_param (p_param TAllStorageParam, p_is_index BOOLEAN := FALSE, p_is_constr BOOLEAN := FALSE) IS
  BEGIN
    FOR bf_tablespace_desc IN cr_tablespace_desc(p_param.tablespace_name) LOOP
      FOR bf_user_desc IN cr_user_desc LOOP
        IF gv_all_params OR p_param.tablespace_name <> bf_user_desc.default_tablespace THEN
          open_using_index_clause(p_is_constr);
          Dbms_Output.Put_Line('  TABLESPACE '||Lower(p_param.tablespace_name));
        END IF;
      END LOOP;
      IF gv_all_params OR p_param.pct_free <> 10 THEN
        open_using_index_clause(p_is_constr);
        Dbms_Output.Put_Line('  PCTFREE  '||LPad(p_param.pct_free,3));
      END IF;
      IF NOT p_is_index AND (gv_all_params OR p_param.pct_used <> 40) THEN
        open_using_index_clause(p_is_constr);
        Dbms_Output.Put_Line('  PCTUSED  '||LPad(p_param.pct_used,3));
      END IF;
      IF gv_all_params
      OR (NOT p_is_index AND p_param.ini_trans <> 1)
      OR (p_is_index AND p_param.ini_trans <> 2) THEN
        Dbms_Output.Put_Line('  INITRANS '||LPad(p_param.ini_trans,3));
      END IF;
      IF gv_all_params OR p_param.max_trans <> 255 THEN
        open_using_index_clause(p_is_constr);
        Dbms_Output.Put_Line('  MAXTRANS '||LPad(p_param.max_trans,3));
      END IF;
      IF gv_all_params OR p_param.initial_extent <> bf_tablespace_desc.initial_extent THEN
        open_using_index_clause(p_is_constr);
        open_storage_clause;
        Dbms_Output.Put_Line('    INITIAL '||LPad(p_param.initial_extent/1024,7)||' K');
      END IF;
      IF gv_all_params OR p_param.next_extent <> bf_tablespace_desc.next_extent THEN
        open_using_index_clause(p_is_constr);
        open_storage_clause;
        Dbms_Output.Put_Line('    NEXT    '||LPad(p_param.next_extent/1024,7)||' K');
      END IF;
      IF gv_all_params OR p_param.min_extents <> bf_tablespace_desc.min_extents THEN
        open_using_index_clause(p_is_constr);
        open_storage_clause;
        Dbms_Output.Put_Line('    MINEXTENTS  '||LPad(p_param.min_extents,3));
      END IF;
      IF gv_all_params OR p_param.max_extents <> bf_tablespace_desc.max_extents THEN
        open_using_index_clause(p_is_constr);
        open_storage_clause;
        Dbms_Output.Put_Line('    MAXEXTENTS  '||LPad(p_param.max_extents,3));
      END IF;
      IF gv_all_params OR p_param.pct_increase <> bf_tablespace_desc.pct_increase THEN
        open_using_index_clause(p_is_constr);
        open_storage_clause;
        Dbms_Output.Put_Line('    PCTINCREASE '||LPad(p_param.pct_increase,3));
      END IF;
      IF gv_all_params OR p_param.freelists > 1 THEN
        open_using_index_clause(p_is_constr);
        open_storage_clause;
        Dbms_Output.Put_Line('    FREELISTS   '||LPad(p_param.freelists,3));
      END IF;
      IF gv_all_params OR p_param.freelist_groups > 1 THEN
        open_using_index_clause(p_is_constr);
        open_storage_clause;
        Dbms_Output.Put_Line('    FREELIST GROUPS '||p_param.freelist_groups);
      END IF;
      close_storage_clause;
      close_using_index_clause;
    END LOOP;
  END;

  PROCEDURE print_columns (p_data_type BOOLEAN := TRUE, p_indent PLS_INTEGER := 2) IS
  BEGIN
    lv_start := TRUE;
    FOR bf_columns IN cr_columns LOOP
      FOR i IN 1..p_indent LOOP
        Dbms_Output.Put(' ');
      END LOOP;
      IF lv_start THEN
        Dbms_Output.Put(' ');
        lv_start := FALSE;
      ELSE
        Dbms_Output.Put(',');
      END IF;
      Dbms_Output.Put(RPad(Lower(bf_columns.column_name),31));
      IF p_data_type THEN
        Dbms_Output.Put(bf_columns.data_type);
        IF bf_columns.data_type IN ('NUMBER','FLOAT') THEN
          IF Nvl(bf_columns.data_precision,0)+Nvl(bf_columns.data_scale,0) > 0 THEN
            Dbms_Output.Put('('||bf_columns.data_precision);
            IF Nvl(bf_columns.data_scale,0) > 0 THEN
               Dbms_Output.Put(','||bf_columns.data_scale);
            END IF;
            Dbms_Output.Put(')');
          END IF;
        ELSIF bf_columns.data_type IN ('DATE','LONG','LONG RAW') THEN
          NULL;
        ELSE
           Dbms_Output.Put('('||bf_columns.data_length||')');
        END IF;
        IF bf_columns.data_default IS NOT NULL THEN
          Dbms_Output.Put(' DEFAULT '||bf_columns.data_default);
        END IF;
        IF bf_columns.nullable = 'N' THEN
          Dbms_Output.Put(' NOT NULL');
        END IF;
      END IF;
      Dbms_Output.New_Line;
    END LOOP;
  END;
/******************************************************************/
BEGIN
  Dbms_Output.Put_Line('REM ---------------------------------------------------------------');
  Dbms_Output.Put_Line('REM -- TABLE TRANSER UTILITY V1.01 BETA Koch 18.10.98 18:49:12   --');
  Dbms_Output.Put_Line('REM -- CAUTION:                                                  --');
  Dbms_Output.Put_Line('REM --   NO WARRANTY OF ANY KIND IS EXPRESSED OR IMPLIED. YOU    --');
  Dbms_Output.Put_Line('REM --   USE AT YOUR OWN RISK. THE AUTHOR WILL NOT BE LIABLE FOR --');
  Dbms_Output.Put_Line('REM --   DATA LOSS, DAMAGES, LOSS OF PROFITS OR ANY OTHER KIND   --');
  Dbms_Output.Put_Line('REM --   OF LOSS WHILE USING OR MISUSING THIS SOFTWARE.          --');
  Dbms_Output.Put_Line('REM ---------------------------------------------------------------');
  Dbms_Output.New_Line;
  Dbms_Output.New_Line;

  FOR bf_table_desc IN cr_table_desc LOOP
    IF bf_table_desc.cluster_name IS NOT NULL THEN
      Raise_Application_Error(-20999,'I am sorry. This table is belong cluster. Reengineering not implement!');
    END IF;
    FOR bf_tab_triggers IN cr_tab_triggers LOOP
      Raise_Application_Error(-20999,'I am sorry. This table has triggers. Reengineering not implement!');
    END LOOP;

    FOR i IN 0..100 LOOP
      IF i = 100 THEN
        Raise_Application_Error(-20999,'Ошибка генерации уникального имени для времменой таблицы!');
      END IF;
      lv_safe_table_name := SubStr(Lower(gv_table_name), 1, 26) || '_$' ||LTrim(To_Char(i,'00'));
      SELECT count(*) INTO lv_counter FROM user_objects
        WHERE object_name = lv_safe_table_name;
      IF lv_counter = 0 THEN
        EXIT;
      END IF;
    END LOOP;

    Dbms_Output.Put_Line('PROMPT -- LOCK TABLE IN EXCLUSIVE MODE');
    Dbms_Output.Put_Line('LOCK TABLE '||Lower(gv_table_name)||' IN EXCLUSIVE MODE NOWAIT');
    Dbms_Output.Put_Line(';');
    Dbms_Output.New_Line;

    Dbms_Output.Put_Line('PROMPT -- RENAME TABLE');
    Dbms_Output.Put_Line('RENAME '||Lower(gv_table_name)||' TO '||lv_safe_table_name);
    Dbms_Output.Put_Line(';');
    Dbms_Output.New_Line;

    Dbms_Output.Put_Line('PROMPT -- DROP NAMED CONSTRAINTS');
    FOR bf_constraints IN cr_constraints LOOP
      IF bf_constraints.table_name = Upper(gv_table_name)
      AND InStr(bf_constraints.constraint_name,'SYS_C') <> 1 THEN
        Dbms_Output.Put_Line('ALTER TABLE '||lv_safe_table_name);
        Dbms_Output.Put('  DROP CONSTRAINT '||Lower(bf_constraints.constraint_name));
        IF bf_constraints.constraint_type IN ('P','U') THEN
          Dbms_Output.Put(' CASCADE');
        END IF;
        Dbms_Output.Put_Line(';');
        Dbms_Output.New_Line;
      END IF;
    END LOOP;
    Dbms_Output.New_Line;

    Dbms_Output.Put_Line('PROMPT -- DROP INDEXES');
    FOR bf_indexes IN cr_indexes LOOP
      Dbms_Output.Put('DROP INDEX '||Lower(bf_indexes.index_name));
      Dbms_Output.Put_Line(';');
      Dbms_Output.New_Line;
    END LOOP;
    Dbms_Output.New_Line;

    Dbms_Output.Put_Line('PROMPT -- CREATE TABLE '||Lower(gv_table_name));
    Dbms_Output.Put_Line('CREATE TABLE '||Lower(gv_table_name)||' (');
    print_columns;
    Dbms_Output.Put_Line(')');

    lv_storage_param.pct_free        := bf_table_desc.pct_free;
    lv_storage_param.pct_used        := bf_table_desc.pct_used;
    lv_storage_param.ini_trans       := bf_table_desc.ini_trans;
    lv_storage_param.max_trans       := bf_table_desc.max_trans;
    lv_storage_param.tablespace_name := bf_table_desc.tablespace_name;
    lv_storage_param.initial_extent  := bf_table_desc.initial_extent;
    lv_storage_param.next_extent     := bf_table_desc.next_extent;
    lv_storage_param.min_extents     := bf_table_desc.min_extents;
    lv_storage_param.max_extents     := bf_table_desc.max_extents;
    lv_storage_param.pct_increase    := bf_table_desc.pct_increase;
    lv_storage_param.freelists       := bf_table_desc.freelists;
    lv_storage_param.freelist_groups := bf_table_desc.freelist_groups;
    print_storage_param(lv_storage_param);

    IF bf_table_desc.cache = 'Y' THEN
      Dbms_Output.Put('  CACHE');
    END IF;
    Dbms_Output.Put_Line(';');
    Dbms_Output.New_Line;

    Dbms_Output.Put_Line('PROMPT -- TRANSFER DATA INTO NEW TABLE');
    Dbms_Output.Put_Line('INSERT INTO '||Lower(gv_table_name)||' (');
    print_columns(FALSE);
    Dbms_Output.Put_Line(') (SELECT');
    print_columns(FALSE,4);
    Dbms_Output.Put_Line('  FROM '||lv_safe_table_name||')');
    Dbms_Output.Put_Line(';');
    Dbms_Output.New_Line;

    Dbms_Output.Put_Line('PROMPT -- BUILD CONSTRAINTS');
    FOR bf_constraints IN cr_constraints LOOP
      SELECT count(*) INTO lv_counter FROM user_tab_columns
        WHERE table_name = Upper(gv_table_name)
        AND (column_name||' IS NOT NULL') = bf_constraints.search_condition;
      IF lv_counter = 0 THEN
        Dbms_Output.Put_Line('ALTER TABLE '||Lower(bf_constraints.table_name));
        Dbms_Output.Put('  ADD');
        IF InStr(bf_constraints.constraint_name,'SYS_C') <> 1 THEN
          Dbms_Output.Put(' CONSTRAINT '||Lower(bf_constraints.constraint_name));
        END IF;
        IF bf_constraints.constraint_type = 'C' THEN
          Dbms_Output.Put_Line(' CHECK (');
          Dbms_Output.Put_Line('  '||bf_constraints.search_condition);
        ELSIF bf_constraints.constraint_type IN ('P','U','R') THEN
          IF bf_constraints.constraint_type = 'P' THEN
            Dbms_Output.Put_Line(' PRIMARY KEY (');
          ELSIF bf_constraints.constraint_type = 'U' THEN
            Dbms_Output.Put_Line(' UNIQUE (');
          ELSE
            Dbms_Output.Put_Line(' FOREIGN KEY (');
          END IF;
          lv_start := TRUE;
          FOR bf_cons_columns IN cr_cons_columns(USER,bf_constraints.constraint_name) LOOP
            Dbms_Output.Put('    ');
            IF lv_start THEN
              lv_start := FALSE;
            ELSE
              Dbms_Output.Put(',');
            END IF;
            Dbms_Output.Put_Line(Lower(bf_cons_columns.column_name));
          END LOOP;
        ELSE NULL;
        END IF;
        IF bf_constraints.constraint_type = 'R' THEN
          Dbms_Output.Put('  ) REFERENCES ');
          FOR bf_ref_constraints IN cr_ref_constraints(bf_constraints.r_owner, bf_constraints.r_constraint_name) LOOP
            IF bf_ref_constraints.owner <> USER THEN -- ? table_owner = constrain_owner
              Dbms_Output.Put(Lower(bf_ref_constraints.owner)||'.');
            END IF;
            Dbms_Output.Put_Line(Lower(bf_ref_constraints.table_name)||' (');
          END LOOP;
          lv_start := TRUE;
          FOR bf_cons_columns IN cr_cons_columns(bf_constraints.r_owner, bf_constraints.r_constraint_name) LOOP
            Dbms_Output.Put('    ');
            IF lv_start THEN
              lv_start := FALSE;
            ELSE
              Dbms_Output.Put(',');
            END IF;
            Dbms_Output.Put_Line(Lower(bf_cons_columns.column_name));
          END LOOP;
          Dbms_Output.Put('  )');
          IF bf_constraints.delete_rule = 'CASCADE' THEN
            Dbms_Output.Put(' ON DELETE CASCADE');
          END IF;
          Dbms_Output.New_Line;
        ELSE
          Dbms_Output.Put_Line('  )');
        END IF;
        IF bf_constraints.constraint_type IN ('P','U') THEN
          FOR bf_constr_indexes IN cr_constr_indexes(bf_constraints.constraint_name) LOOP
            lv_storage_param.pct_free        := bf_constr_indexes.pct_free;
            lv_storage_param.pct_used        := NULL;
            lv_storage_param.ini_trans       := bf_constr_indexes.ini_trans;
            lv_storage_param.max_trans       := bf_constr_indexes.max_trans;
            lv_storage_param.tablespace_name := bf_constr_indexes.tablespace_name;
            lv_storage_param.initial_extent  := bf_constr_indexes.initial_extent;
            lv_storage_param.next_extent     := bf_constr_indexes.next_extent;
            lv_storage_param.min_extents     := bf_constr_indexes.min_extents;
            lv_storage_param.max_extents     := bf_constr_indexes.max_extents;
            lv_storage_param.pct_increase    := bf_constr_indexes.pct_increase;
            lv_storage_param.freelists       := bf_constr_indexes.freelists;
            lv_storage_param.freelist_groups := bf_constr_indexes.freelist_groups;
            print_storage_param(lv_storage_param,TRUE,TRUE);
          END LOOP;
        END IF;
        IF bf_constraints.status = 'DISABLED' THEN
          Dbms_Output.Put_Line('  DISABLE');
        END IF;
        Dbms_Output.Put_Line(';');
        Dbms_Output.New_Line;
      END IF;
    END LOOP;

    Dbms_Output.Put_Line('PROMPT -- BUILD INDEXES');
    FOR bf_indexes IN cr_indexes LOOP
      Dbms_Output.Put('CREATE');
      IF bf_indexes.uniqueness = 'UNIQUE' THEN
        Dbms_Output.Put(' UNIQUE');
      END IF;
      Dbms_Output.Put_Line(' INDEX '||Lower(bf_indexes.index_name));
      Dbms_Output.Put_Line('  ON '||Lower(gv_table_name)||' (');
      lv_start := TRUE;
      FOR bf_ind_columns IN cr_ind_columns(bf_indexes.index_name) LOOP
        Dbms_Output.Put('    ');
        IF lv_start THEN
          lv_start := FALSE;
        ELSE
          Dbms_Output.Put(',');
        END IF;
        Dbms_Output.Put_Line(Lower(bf_ind_columns.column_name));
      END LOOP;
      Dbms_Output.Put_Line('  )');

      lv_storage_param.pct_free        := bf_indexes.pct_free;
      lv_storage_param.pct_used        := NULL;
      lv_storage_param.ini_trans       := bf_indexes.ini_trans;
      lv_storage_param.max_trans       := bf_indexes.max_trans;
      lv_storage_param.tablespace_name := bf_indexes.tablespace_name;
      lv_storage_param.initial_extent  := bf_indexes.initial_extent;
      lv_storage_param.next_extent     := bf_indexes.next_extent;
      lv_storage_param.min_extents     := bf_indexes.min_extents;
      lv_storage_param.max_extents     := bf_indexes.max_extents;
      lv_storage_param.pct_increase    := bf_indexes.pct_increase;
      lv_storage_param.freelists       := bf_indexes.freelists;
      lv_storage_param.freelist_groups := bf_indexes.freelist_groups;
      print_storage_param(lv_storage_param,TRUE);

      Dbms_Output.Put_Line(';');
      Dbms_Output.New_Line;
    END LOOP;

    Dbms_Output.Put_Line('PROMPT -- ADD COMMENTS');
    FOR bf_tab_comments IN cr_tab_comments LOOP
      Dbms_Output.Put('COMMENT ON TABLE '||Lower(bf_tab_comments.table_name));
      Dbms_Output.Put_Line(' IS '''||bf_tab_comments.comments||''';');
      Dbms_Output.New_Line;
    END LOOP;
    FOR bf_col_comments IN cr_col_comments LOOP
      Dbms_Output.Put('COMMENT ON COLUMN '||Lower(bf_col_comments.table_name)||'.'||Lower(bf_col_comments.column_name));
      Dbms_Output.Put_Line(' IS '''||bf_col_comments.comments||''';');
      Dbms_Output.New_Line;
    END LOOP;

    Dbms_Output.Put_Line('PROMPT -- ADD TABLE GRANTS');
    Dbms_Output.New_Line;
    FOR bf_tab_grants IN cr_tab_grants LOOP
      Dbms_Output.Put('GRANT '||bf_tab_grants.privilege);
      Dbms_Output.Put(' ON '||bf_tab_grants.table_name);
      Dbms_Output.Put_Line(' TO '||bf_tab_grants.grantee||';');
      Dbms_Output.New_Line;
    END LOOP;

    Dbms_Output.Put_Line('PROMPT -- ADD COLUMNS GRANTS');
    Dbms_Output.New_Line;
    FOR bf_col_grants IN cr_col_grants LOOP
      Dbms_Output.Put('GRANT '||bf_col_grants.privilege);
      Dbms_Output.Put(' ON '||bf_col_grants.table_name||'.'||bf_col_grants.column_name);
      Dbms_Output.Put_Line(' TO '||bf_col_grants.grantee||';');
      Dbms_Output.New_Line;
    END LOOP;

    Dbms_Output.Put_Line('PROMPT -- !!! DROP OLD TABLE !!!');
    Dbms_Output.Put_Line('--DROP TABLE '||lv_safe_table_name);
    Dbms_Output.Put_Line(';');
    Dbms_Output.New_Line;

  END LOOP;
END;
