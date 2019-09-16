-- View: public.v_runningstats
CREATE OR REPLACE VIEW public.v_runningstats AS
 SELECT running.id,
    running.dayselapsed,
    running.dayselapsed / 365 AS years,
    running.popuation,
    running.seasonalfactor,
    running.endtime - running.starttime AS "time"
   FROM ( SELECT md.id,
            md.seasonalfactor,
            md.dayselapsed,
            md.entrytime AS endtime,
            ( SELECT monthlydata.entrytime
                   FROM sim.monthlydata
                  WHERE monthlydata.id = (md.id - 1)) AS starttime,
            ( SELECT sum(monthlysitedata.population) AS population
                   FROM sim.monthlysitedata
                  WHERE monthlysitedata.monthlydataid = md.id) AS popuation
           FROM sim.monthlydata md) running
  ORDER BY running.id;

ALTER TABLE public.v_runningstats
    OWNER TO postgres;