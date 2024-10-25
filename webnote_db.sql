CREATE EXTENSION WHEN NOT EXISTS pgcrypto WITH SCHEMA public;

COMMENT ON EXTENSION pgcrypto IS 'cryptographic functions';

-- References:
-- https://www.postgresql.org/docs/9.1/sql-createtrigger.html
-- https://stackoverflow.com/q/61917751
CREATE FUNCTION public.fn_trig_notes_id() RETURNS trigger
  LANGUAGE plpgsql
  AS $$
  BEGIN
    new.id = (SELECT count (*) FROM datawebnote WHERE username = new.username);
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

CREATE TABLE public.userwebnote (
  username character varying(40) NOT NULL,
  password character varying(100) NOT NULL,
  account_birth timestamp with time zone DEFAULT now() NOT NULL
);

ALTER TABLE public.userwebnote OWNER TO hitagi;

ALTER TABLE ONLY public.userwebnote
  ADD CONSTRAINT userwebnote_pkey PRIMARY KEY (id, username);

ALTER TABLE ONLY public.userwebnote
   ADD CONSTRAINT unique_username UNIQUE (username);

-- Function trigger before insert for auto generate new id
CREATE TRIGGER trig_notes_id
  BEFORE INSERT
  ON public.datawebnote
  FOR EACH ROW
  EXECUTE PROCEDURE public.fn_trig_notes_id();

ALTER TABLE ONLY public.datawebnote
    ADD CONSTRAINT datawebnote_username_fkey FOREIGN KEY (username) REFERENCES public.userwebnote(username);
