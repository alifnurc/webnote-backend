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
