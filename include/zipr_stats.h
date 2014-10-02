#ifndef zipr_stats_t
#define zipr_stats_t

class Stats_t
{
	public:
		Stats_t() {
			for (int i=0; i<Optimizations_t::NumberOfOptimizations; i++)
			{
				Hits[i] = Misses[i] = 0;
			}
		};
		int Hits[Optimizations_t::NumberOfOptimizations];
		int Misses[Optimizations_t::NumberOfOptimizations];
		void Hit(Optimizations_t::OptimizationName_t opt) { Hits[opt]++; };
		void Missed(Optimizations_t::OptimizationName_t opt) { Misses[opt]++; };
};
#endif
