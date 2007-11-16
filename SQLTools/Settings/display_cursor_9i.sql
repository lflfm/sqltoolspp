declare
  -- cursor c_display_cursor(in_address varchar2, in_hash_value number, in_child_number number) is
  s_display_cursor varchar2(32767) := '
  select case when access_predicates is not null or filter_predicates is not null then ''*'' else '' '' end || substr(to_char(id, ''999''), -3) as p_id,
  lpad('' '', depth - 1) || operation || '' '' || options as operation,
  object_name as name,
  starts,
  cardinality as e_rows,
  outrows as a_rows,
  to_char(to_number(substr(e_time_interval, 2, 9)) * 24 + to_number(substr(e_time_interval, 12, 2)), ''FM900'') || substr(e_time_interval, 14, 9) as a_time,
  crgets + cugets as buffers,
  case reads when 0 then null else reads end as reads,
  case writes when 0 then null else writes end as writes,
  case other_tag
  when ''PARALLEL_TO_SERIAL'' then ''P->S''
  when ''PARALLEL_COMBINED_WITH_PARENT'' then ''PCWP''
  when ''PARALLEL_COMBINED_WITH_CHILD'' then ''PCWC''
  when ''SERIAL_TO_PARALLEL'' then ''S->P''
  when ''PARALLEL_TO_PARALLEL'' then ''P->P''
  else other_tag end as in_out,
  partition_start,
  partition_stop,
  distribution,
  mem_opt,
  mem_one,
  last_mem_used || case last_mem_usage when ''OPTIMAL'' then '' (0)'' when ''ONE PASS'' then '' (1)'' when ''MULTI-PASS'' then '' (M)'' end,
  case last_degree when 1 then null else last_degree end as last_degree,
  --opt_cnt,
  --one_cnt,
  --multi_cnt,
  --max_tmp,
  last_tmp,
  access_predicates,
  filter_predicates,
  plan_hash_value,
  dynamic_sampling_flag
  from (
  SELECT /*+ opt_param(''parallel_execution_enabled'', ''false'') */
                        id, position, depth , operation, options, object_name, cardinality, bytes, temp_space, cost, io_cost,
                        cpu_cost, partition_start, partition_stop, object_node, other_tag, distribution, null, access_predicates, filter_predicates,
                        null, null, null, starts, outrows, crgets, cugets, reads, writes, etime, e_time_interval, mem_opt, mem_one, last_mem_used, last_degree, last_mem_usage,
                        opt_cnt, one_cnt, multi_cnt, max_tmp, last_tmp, plan_hash_value, dynamic_sampling_flag from (
                            select /*+ no_merge */
                            id, depth, position, operation, options,
                            cost, cardinality, bytes, object_node,
                            object_name, other_tag, partition_start,
                            partition_stop, distribution, temp_space, io_cost,
                            cpu_cost, filter_predicates, access_predicates, other,
                            last_starts starts,
                                      last_output_rows outrows,
                                      last_cr_buffer_gets crgets,
                                      last_cu_buffer_gets cugets,
                                      last_disk_reads reads,
                                      last_disk_writes writes,
                                      last_elapsed_time etime,
                                      to_char(numtodsinterval(round(last_elapsed_time/10000)*10000/1000000, ''SECOND'')) as e_time_interval,
                                      estimated_optimal_size mem_opt,
                                      estimated_onepass_size mem_one,
                                      last_memory_used last_mem_used,
                                      last_degree,
                                      last_execution last_mem_usage,
                                      optimal_executions opt_cnt,
                                      onepass_executions one_cnt,
                                      multipasses_executions multi_cnt,
                                      max_tempseg_size max_tmp,
                                      last_tempseg_size last_tmp,
                                      plan_hash_value,
                                      case 0 /*instr(other_xml, ''dynamic_sampling'')*/ when 0 then NULL else ''YES'' end as dynamic_sampling_flag
                from V$SQL_PLAN_STATISTICS_ALL vp
                where address = hextoraw(:in_address)
                and hash_value = :in_hash_value
                and child_number = :in_child_number)
  )
  order by id';
  type t_list_varchar2 is table of varchar2(4000) index by pls_integer;
  type t_column_record is record (
  a_data        t_list_varchar2,
  b_has_data    boolean,
  s_heading     varchar2(255),
  b_is_number   boolean default false,
  s_alignment   varchar2(20),
  n_max_size    pls_integer);
  type t_column_list is table of t_column_record index by pls_integer;
  a_column_list t_column_list;
  n_row_size    pls_integer;
  s_row         varchar2(4000);
  a_access_pred t_list_varchar2;
  a_filter_pred t_list_varchar2;
  a_plan_hash   t_list_varchar2;
  a_dyn_sampl   t_list_varchar2;
  s_output      varchar2(4000);
  s_sql_address varchar2(255);
  s_hash_value  varchar2(255);
  s_child_num   varchar2(255);
  max_line_size constant pls_integer := 255;
  c_display_cursor sys_refcursor;
  function has_collection_only_nulls(in_coll in t_list_varchar2)
  return boolean is
    b_return boolean := true;
  begin
    if in_coll.count > 0 then
      for i in in_coll.first..in_coll.last loop
        if in_coll(i) is not null then
          b_return := false;
          exit;
        end if;
      end loop;
    end if;
    return b_return;
  end has_collection_only_nulls;
  function get_max_size(in_coll in t_list_varchar2)
  return pls_integer is
    n_return pls_integer := 0;
  begin
    if in_coll.count > 0 then
      for i in in_coll.first..in_coll.last loop
        if in_coll(i) is not null then
          n_return := greatest(n_return, length(in_coll(i)));
        end if;
      end loop;
    end if;
    return n_return;
  end get_max_size;
  function display_cursor_format_number(in_data in varchar2)
  return varchar2 is
    s_return varchar2(20);
    s_trail varchar2(32767);
    s_data varchar2(32767);
    n_number number;
    n_delim_pos number;
  begin
    n_delim_pos := instr(in_data, ' ');
    if n_delim_pos > 0 then
      s_trail := substr(in_data, n_delim_pos);
      s_data := substr(in_data, 1, n_delim_pos - 1);
    else
      s_data := in_data;
    end if;
    n_number := to_number(s_data);
    s_return :=
    case
    when n_number >= 100000000000000000000 then to_char(n_number/1000000000000000000, 'FM99999') || 'E'
    when n_number >= 100000000000000000 then to_char(n_number/1000000000000000, 'FM99999') || 'P'
    when n_number >= 100000000000000 then to_char(n_number/1000000000000, 'FM99999') || 'T'
    when n_number >= 100000000000 then to_char(n_number/1000000000, 'FM99999') || 'G'
    when n_number >= 100000000 then to_char(n_number/1000000, 'FM99999') || 'M'
    when n_number >= 100000 then to_char(n_number/1000, 'FM99999') || 'K'
    else to_char(n_number, 'FM99999')
    end;
    return s_return || s_trail;
  end display_cursor_format_number;
  procedure put_line_smart(in_string in varchar2, in_line_prefix in varchar2 default '', in_line_size in pls_integer default 134) is
    n_offset pls_integer;
    s_delimiter varchar2(1);
    n_size_current_line pls_integer;
    n_line_counter pls_integer;
  begin
    n_offset := 1;
    n_size_current_line := in_line_size;
    n_line_counter := 1;
    while case when n_line_counter > 1 and length(in_line_prefix) > 0
          then length(in_string) + length(in_line_prefix)
          else length(in_string) end
          + 1 - n_offset > in_line_size loop
      -- dbms_output.put_line('Debug n_offset: ' || n_offset);
      if n_line_counter > 1 and length(in_line_prefix) > 0 then
        n_size_current_line := greatest(n_size_current_line - length(in_line_prefix), length(in_line_prefix) + 10);
      end if;
      -- dbms_output.put_line('Debug n_size_current_line: ' || n_size_current_line);
      loop
        s_delimiter := substr(in_string, n_offset - 1 + n_size_current_line, 1);
        exit when s_delimiter in (' ', chr(9), chr(10), chr(13)/*, '(', ')', '[', ']'*/) or n_size_current_line < 1;
        n_size_current_line := n_size_current_line - 1;
      end loop;
      if n_size_current_line < 1 then
        if n_line_counter > 1 and length(in_line_prefix) > 0 then
          n_size_current_line := greatest(n_size_current_line - length(in_line_prefix), length(in_line_prefix) + 10);
        else
          n_size_current_line := in_line_size;
        end if;
      end if;
      if s_delimiter in (chr(13), chr(10)) then
        n_size_current_line := n_size_current_line - 1;
      end if;
      dbms_output.put_line(case when n_line_counter > 1 then in_line_prefix end || substr(in_string, n_offset, n_size_current_line));
      if s_delimiter in (chr(13), chr(10)) then
        while substr(in_string, n_offset - 1 + n_size_current_line, 1) in (chr(10), chr(13)) loop
          n_size_current_line := n_size_current_line + 1;
        end loop;
      end if;
      n_offset := n_offset + n_size_current_line;
      n_size_current_line := in_line_size;
      n_line_counter := n_line_counter + 1;
    end loop;
    dbms_output.put_line(case when n_line_counter > 1 then in_line_prefix end || substr(in_string, n_offset));
  end put_line_smart;
begin
  s_sql_address := :sql_address;
  s_hash_value := :hash_value;
  s_child_num := 0;
  -- Header information
  put_line_smart(in_string => 'SQL address: ' || s_sql_address || ' hash_value: ' || s_hash_value || ' child_number: ' || s_child_num || ' (assumed, SQL could be the wrong one, "child_number" not part of V$SESSION in 9i)', in_line_size => max_line_size);
  put_line_smart(in_string => '------------------------------------------------------------------------------------------------------------------------------------------------------', in_line_size => max_line_size);
  begin
  	execute immediate '
    select sql_text
    from v$sql
    where address = :s_sql_address
    and hash_value = to_number(:s_hash_value)
    and child_number = to_number(:s_child_num)'
    into s_output using s_sql_address, s_hash_value, s_child_num;
  exception
  when NO_DATA_FOUND then
    null;
  when others then
    dbms_output.put_line('Error getting SQL text from V$SQL, check privileges');
  end;
  put_line_smart(s_output);
  dbms_output.put_line('');
  -- The plan statistics
  a_column_list(1).s_heading := 'Id'; --a_column_list(1).b_is_number := true;
  a_column_list(2).s_heading := 'Operation';
  a_column_list(3).s_heading := 'Name';
  a_column_list(4).s_heading := 'Starts'; a_column_list(4).b_is_number := true;
  a_column_list(5).s_heading := 'E-Rows'; a_column_list(5).b_is_number := true;
  a_column_list(6).s_heading := 'A-Rows'; a_column_list(6).b_is_number := true;
  a_column_list(7).s_heading := 'A-Time';
  a_column_list(8).s_heading := 'Buffers'; a_column_list(8).b_is_number := true;
  a_column_list(9).s_heading := 'Reads'; a_column_list(9).b_is_number := true;
  a_column_list(10).s_heading := 'Writes'; a_column_list(10).b_is_number := true;
  a_column_list(11).s_heading := 'In-Out';
  a_column_list(12).s_heading := 'Pstart'; a_column_list(12).b_is_number := true;
  a_column_list(13).s_heading := 'Pstop'; a_column_list(13).b_is_number := true;
  a_column_list(14).s_heading := 'PQ Distrib';
  a_column_list(15).s_heading := 'OMem'; a_column_list(15).b_is_number := true;
  a_column_list(16).s_heading := '1Mem'; a_column_list(16).b_is_number := true;
  a_column_list(17).s_heading := 'Used-Mem'; a_column_list(17).b_is_number := true; --a_column_list(15).s_alignment := 'RIGHT';
  a_column_list(18).s_heading := 'Last-Degree'; a_column_list(18).b_is_number := true;
  --a_column_list(19).s_heading := 'Opt-Cnt'; a_column_list(17).b_is_number := true;
  --a_column_list(20).s_heading := 'One-Cnt'; a_column_list(18).b_is_number := true;
  --a_column_list(21).s_heading := 'Multi-Cnt'; a_column_list(19).b_is_number := true;
  --a_column_list(19).s_heading := 'Max-Tmp'; a_column_list(19).b_is_number := true;
  a_column_list(19).s_heading := 'Last-Tmp'; a_column_list(19).b_is_number := true;
  open c_display_cursor for s_display_cursor using s_sql_address, to_number(s_hash_value), to_number(s_child_num);
  fetch c_display_cursor bulk collect into
  a_column_list(1).a_data, -- a_p_id,
  a_column_list(2).a_data, -- a_operation,
  a_column_list(3).a_data, -- a_name,
  a_column_list(4).a_data, -- a_starts,
  a_column_list(5).a_data, -- a_e_rows,
  a_column_list(6).a_data, -- a_a_rows,
  a_column_list(7).a_data, -- a_a_time,
  a_column_list(8).a_data, -- a_buffers,
  a_column_list(9).a_data, -- a_reads,
  a_column_list(10).a_data, -- a_writes,
  a_column_list(11).a_data, -- a_in_out,
  a_column_list(12).a_data, -- a_partition_start,
  a_column_list(13).a_data, -- a_partition_stop,
  a_column_list(14).a_data, -- a_distribution,
  a_column_list(15).a_data, -- a_last_mem_usage,
  a_column_list(16).a_data, -- a_last_degree,
  a_column_list(17).a_data, -- a_mem_opt,
  a_column_list(18).a_data, -- a_mem_one,
  --a_column_list(17).a_data, -- a_opt_cnt,
  --a_column_list(18).a_data, -- a_one_cnt,
  --a_column_list(19).a_data, -- a_multi_cnt,
  --a_column_list(22).a_data, -- a_max_tmp,
  a_column_list(19).a_data, -- a_last_tmp,
  a_access_pred,
  a_filter_pred,
  a_plan_hash,
  a_dyn_sampl;
  close c_display_cursor;
  if a_column_list(1).a_data.count > 0 then
    dbms_output.put_line('Plan hash value: ' || a_plan_hash(1));
    dbms_output.put_line('');
    n_row_size := 1;
    for i in a_column_list.first..a_column_list.last loop
      if a_column_list(i).b_is_number then
        if a_column_list(i).a_data.count > 0 then
          for j in a_column_list(i).a_data.first..a_column_list(i).a_data.last loop
            begin
              a_column_list(i).a_data(j) := display_cursor_format_number(a_column_list(i).a_data(j));
            exception
            when others then
              dbms_output.put_line('Column:' || a_column_list(i).s_heading || ' Data: ' || a_column_list(i).a_data(j));
              raise;
            end;
          end loop;
        end if;
      end if;
      -- column size is greatest of max size of content + 2 (leading + trailing blanks) and size of column heading
      a_column_list(i).n_max_size := greatest(get_max_size(a_column_list(i).a_data) + 2, length(a_column_list(i).s_heading) + 2);
      a_column_list(i).b_has_data := not has_collection_only_nulls(a_column_list(i).a_data);
      if a_column_list(i).b_has_data then
        n_row_size := n_row_size + a_column_list(i).n_max_size + 1;
      end if;
    end loop;
    -- Header
    put_line_smart(in_string => lpad('-', n_row_size, '-'), in_line_size => max_line_size);
    s_row := '';
    for i in a_column_list.first..a_column_list.last loop
      if a_column_list(i).b_has_data then
        if a_column_list(i).s_alignment is null then
          if a_column_list(i).b_is_number then
            s_row := s_row || '|' || lpad(a_column_list(i).s_heading, a_column_list(i).n_max_size - 1) || ' ';
          else
            s_row := s_row || '|' || ' ' || rpad(a_column_list(i).s_heading, a_column_list(i).n_max_size - 1);
          end if;
        else
          if a_column_list(i).s_alignment = 'RIGHT' then
            s_row := s_row || '|' || lpad(a_column_list(i).s_heading, a_column_list(i).n_max_size - 1) || ' ';
          else
            s_row := s_row || '|' || ' ' || rpad(a_column_list(i).s_heading, a_column_list(i).n_max_size - 1);
          end if;
        end if;
      end if;
    end loop;
    s_row := s_row || '|';
    put_line_smart(in_string => s_row, in_line_size => max_line_size);
    -- Data
    put_line_smart(in_string => lpad('-', n_row_size, '-'), in_line_size => max_line_size);
    for j in a_column_list(1).a_data.first..a_column_list(1).a_data.last loop
      s_row := '';
      for i in a_column_list.first..a_column_list.last loop
        if a_column_list(i).b_has_data then
          if a_column_list(i).b_is_number then
            s_row := s_row || '|' || lpad(nvl(a_column_list(i).a_data(j), ' '), a_column_list(i).n_max_size - 1) || ' ';
          else
            s_row := s_row || '|' || ' ' || rpad(nvl(a_column_list(i).a_data(j), ' '), a_column_list(i).n_max_size - 1);
          end if;
        end if;
      end loop;
      s_row := s_row || '|';
      put_line_smart(in_string => s_row, in_line_size => max_line_size);
    end loop;
    -- Footer
    put_line_smart(in_string => lpad('-', n_row_size, '-'), in_line_size => max_line_size);
    -- Predicate information
    dbms_output.put_line('');
    dbms_output.put_line('Predicate Information (identified by operation id):');
    dbms_output.put_line('---------------------------------------------------');
    dbms_output.put_line('');
    for j in a_column_list(1).a_data.first..a_column_list(1).a_data.last loop
      if a_access_pred(j) is not null or a_filter_pred(j) is not null then
        s_output := lpad(to_char(j, 'FM9999'), 4, ' ') || ' - ';
        if a_access_pred(j) is not null then
          put_line_smart(s_output || 'access(' || a_access_pred(j) || ')', lpad(' ', length(s_output), ' '));
        end if;
        if a_filter_pred(j) is not null then
          if a_access_pred(j) is not null then
            put_line_smart(lpad(' ', length(s_output), ' ') || 'filter(' || a_filter_pred(j) || ')', lpad(' ', length(s_output), ' '));
          else
            put_line_smart(s_output || 'filter(' || a_filter_pred(j) || ')', lpad(' ', length(s_output), ' '));
          end if;
        end if;
      end if;
    end loop;
    -- Notes section
    if not a_column_list(4).b_has_data or a_dyn_sampl(1) = 'YES' then
      dbms_output.put_line('');
      dbms_output.put_line('Note');
      dbms_output.put_line('-----');
    end if;
    if a_dyn_sampl(1) = 'YES' then
      dbms_output.put_line('   - dynamic sampling used for this statement');
    end if;
    if not a_column_list(4).b_has_data then
      dbms_output.put_line('   - Warning: basic plan statistics not available. These are only collected when:');
      dbms_output.put_line('       * parameter ''statistics_level'' is set to ''ALL'', at session or system level');
    end if;
  else
    dbms_output.put_line('SQL information could not be found for SQL address: ' || s_sql_address || ' hash_value: ' || s_hash_value || ' child_number: ' || s_child_num);
    dbms_output.put_line('Please verify value of SQL address, hash_value and child_number;');
    dbms_output.put_line('It could also be that the plan is no longer in cursor cache (check v$sql_plan)');
  end if;
exception
when others then
  dbms_output.put_line('Error getting plan information from V$SQL and V$SQL_PLAN_STATISTICS_ALL, check privileges.');
  dbms_output.put_line('You need SELECT privileges on V$SQL_PLAN_STATISTICS_ALL and V$SQL.');
end;
