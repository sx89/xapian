/* stats.h: Handling of statistics needed for the search.
 *
 * ----START-LICENCE----
 * Copyright 1999,2000 Dialog Corporation
 * 
 * This program is free software; you can redistribute it and/or 
 * modify it under the terms of the GNU General Public License as 
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
 * USA
 * -----END-LICENCE-----
 */

#ifndef OM_HGUARD_STATS_H
#define OM_HGUARD_STATS_H

#include "om/omtypes.h"
#include "omassert.h"
#include <map>

/** Class to hold statistics for a given collection. */
class Stats {
    public:
	/** Number of documents in the collection. */
	om_doccount collection_size;

	/** Number of relevant documents in the collection. */
	om_doccount rset_size;

	/** Map of term frequencies for the collection. */
	map<om_termname, om_doccount> termfreq;

	/** Map of relevant term frequencies for the collection. */
	map<om_termname, om_doccount> reltermfreq;


	Stats() : collection_size(0),
		  rset_size(0) {}

	/** Add in the supplied statistics from a sub-database.
	 */
	Stats & operator +=(const Stats & inc);
};

/** Statistics collector: gathers statistics from sub-databases, puts them 
 *  together to build statistics for whole collection, and returns the
 *  overall statistics to each sub-database.
 */
class StatsGatherer {
    private:
	/** Flag to say that statistics have been gathered.
	 *  Some entries in stats are only valid after this.
	 *  Stats should also not be modified before this.
	 */
	mutable bool have_gathered;

	/** The total statistics gathered for the whole collection.
	 */
	Stats total_stats;
    public:
	StatsGatherer();

	/** Set the global collection statistics.
	 *  Should be called before the match is performed.
	 */
	void set_global_stats(om_doccount collection_size,
			      om_doccount rset_size);

	/** Contribute some statistics to the overall statistics.
	 *  Should only be called once by each sub-database.
	 */
	void contrib_stats(const Stats & extra_stats);

	/** Get the collected statistics for the whole collection.
	 */
	const Stats * get_stats() const;
};

/** Statistics leaf: gathers notifications of statistics which will be
 *  needed, and passes them on in bulk.  There is one of these for each
 *  LeafMatch.
 */
class StatsLeaf {
    private:
	/** The gatherer which we report our information to, and ask
	 *  for the global information from.
	 */
	StatsGatherer * gatherer;

	/** The stats to give to the StatsGatherer.
	 */
	Stats my_stats;

	/** The collection statistics, held by the StatsGatherer.
	 *  0 before these have been retrieved.
	 */
	mutable const Stats * total_stats;
	
	/** Perform the request for the needed information.  This involves
	 *  passing our information to the gatherer, and then getting the
	 *  result back.
	 */
	void perform_request() const;
    public:
	/// Constructor: takes the gatherer to talk to.
	StatsLeaf(StatsGatherer * gatherer_);

	/** Set the term frequency in the sub-database which this stats
	 *  object represents.  This is the number of documents in
	 *  the sub-database indexed by the given term.
	 */
	void my_termfreq_is(om_termname & tname, om_doccount tfreq);

	/** Set the relevant term-frequency in the sub-database which this
	 *  stats object represents.  This is the number of relevant
	 *  documents in the sub-database indexed by the given term.
	 */
	void my_reltermfreq_is(om_termname & tname, om_doccount rtfreq);

	/** Get the term frequency over the whole collection, for the
	 *  given term.  This is "n_t", the number of documents in the
	 *  collection indexed by the given term.  (May be an approximation.)
	 */
	om_doccount get_total_termfreq(om_termname & tname) const;

	/** Get the relevant term-frequency over the whole collection, for
	 *  the given term.  This is "r_t", the number of relevant documents
	 *  in the collection indexed by the given term.  (May be an
	 *  approximation.)
	 */
	om_doccount get_total_reltermfreq(om_termname & tname) const;
};

///////////////////////////////
// Inline method definitions //
///////////////////////////////

inline Stats &
Stats::operator +=(const Stats & inc)
{
    // Add the collection size in.
    collection_size += inc.collection_size;

    // Rset size is not passed up.
    Assert(inc.rset_size == 0);

    // Add termfreqs
    map<om_termname, om_doccount>::const_iterator i;
    for(i = inc.termfreq.begin(); i != inc.termfreq.end(); i++) {
	termfreq[i->first] += i->second;
    }
    for(i = inc.reltermfreq.begin(); i != inc.reltermfreq.end(); i++) {
	reltermfreq[i->first] += i->second;
    }
    return *this;
}

inline
StatsGatherer::StatsGatherer()
	: have_gathered(false)
{}

inline void
StatsGatherer::set_global_stats(om_doccount collection_size,
				om_doccount rset_size)
{
    total_stats.collection_size = collection_size;
    total_stats.rset_size = rset_size;
}

inline
StatsLeaf::StatsLeaf(StatsGatherer * gatherer_)
	: gatherer(gatherer_), total_stats(0)
{
}

inline void
StatsLeaf::my_termfreq_is(om_termname & tname, om_doccount tfreq)
{
    Assert(total_stats == 0);
    Assert(my_stats.termfreq.find(tname) == my_stats.termfreq.end());
    my_stats.termfreq[tname] = tfreq;
}

inline void
StatsLeaf::my_reltermfreq_is(om_termname & tname, om_doccount rtfreq)
{   
    Assert(total_stats == 0);
    Assert(my_stats.reltermfreq.find(tname) == my_stats.reltermfreq.end());
    my_stats.reltermfreq[tname] = rtfreq;
}

inline om_doccount
StatsLeaf::get_total_termfreq(om_termname & tname) const
{
    if(total_stats == 0) perform_request();
    map<om_termname, om_doccount>::const_iterator tfreq;
    tfreq = total_stats->termfreq.find(tname);
    Assert(tfreq != total_stats->termfreq.end());
    return tfreq->second;
}

inline om_doccount
StatsLeaf::get_total_reltermfreq(om_termname & tname) const
{
    if(total_stats == 0) perform_request();
    map<om_termname, om_doccount>::const_iterator rtfreq;
    rtfreq = total_stats->reltermfreq.find(tname);
    Assert(rtfreq != total_stats->reltermfreq.end());
    return rtfreq->second;
}

#endif /* OM_HGUARD_STATS_H */
