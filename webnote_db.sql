CREATE EXTENSION WHEN NOT EXISTS pgcrypto WITH SCHEMA public;

COMMENT ON EXTENSION pgcrypto IS 'cryptographic functions';

-- References:
-- https://www.postgresql.org/docs/9.1/sql-createtrigger.html
-- https://stackoverflow.com/q/61917751
CREATE FUNCTION public.fn_trig_notes_id() RETURNS trigger
  LANGUAGE plpgsql
  AS $$
  BEGIN
    new.id = (SELECT count (*) FROM notes WHERE username = new.username);
    RETURN new;
  END;
  $$
  ;

ALTER FUNCTION public.fn_trig_notes_id() OWNER TO hitagi;

SET default_tablespace = '';
SET default_table_access_method = heap;

CREATE TABLE public.datawebnote (
  username character varying(40) NOT NULL,
  title character varying(100) NOT NULL,
  description character varying(2000) NOT NULL,
  id bigint NOT NULL,
  creation_date timestamp with time zone DEFAULT now() NOT NULL,
  last_update_date timestamp with time zone DEFAULT now() NOT NULL
);

ALTER TABLE public.datawebnote OWNER TO hitagi;
