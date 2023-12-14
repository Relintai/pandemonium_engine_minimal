#ifndef WAVE_FORM_COLLAPSE_H
#define WAVE_FORM_COLLAPSE_H

#include "core/int_types.h"
#include "core/math/random_pcg.h"

#include "array_2d.h"
#include "array_3d.h"
#include "core/containers/vector.h"

#include "core/object/reference.h"

class WaveFormCollapse : public Reference {
	GDCLASS(WaveFormCollapse, Reference);

public:
	enum Symmetry {
		SYMMETRY_X = 0,
		SYMMETRY_T,
		SYMMETRY_I,
		SYMMETRY_L,
		SYMMETRY_BACKSLASH,
		SYMMETRY_P
	};

	enum ObserveStatus {
		OBSERVE_STATUS_SUCCESS = 0,
		OBSERVE_STATUS_FAILURE,
		OBSERVE_STATUS_TO_CONTINUE
	};

	struct PropagatorStateEntry {
		Vector<int> directions[4];
	};

	struct PropagatingEntry {
		int data[3];

		PropagatingEntry() {
			for (int i = 0; i < 3; ++i) {
				data[i] = 0;
			}
		}

		PropagatingEntry(int x, int y, int z) {
			data[0] = x;
			data[1] = y;
			data[2] = z;
		}
	};

	struct CompatibilityEntry {
		int direction[4];

		CompatibilityEntry() {
			for (int i = 0; i < 4; ++i) {
				direction[i] = 0;
			}
		}
	};

	static const int DIRECTIONS_X[4];
	static const int DIRECTIONS_Y[4];

public:
	int get_wave_width() const;
	void set_wave_width(const int val);

	int get_wave_height() const;
	void set_wave_height(const int val);

	bool get_periodic_output() const;
	void set_periodic_output(const bool val);

	void set_seed(const int seed);

	void set_wave_size(int p_width, int p_height);
	void init_wave();

	void set_propagator_state(const Vector<PropagatorStateEntry> &p_propagator_state);
	void set_pattern_frequencies(const Vector<double> &p_patterns_frequencies, const bool p_normalize = true);

	virtual void set_input(const PoolIntArray &p_data, int p_width, int p_height);

	virtual Array2D<int> run();

	PoolIntArray generate_image_index_data();

	ObserveStatus observe();

	void remove_wave_pattern(int i, int j, int pattern) {
		if (wave_get(i, j, pattern)) {
			wave_set(i, j, pattern, false);
			add_to_propagator(i, j, pattern);
		}
	}

	// Return true if pattern can be placed in cell index.
	bool wave_get(int index, int pattern) const {
		return _wave_data.get(index, pattern);
	}

	// Return true if pattern can be placed in cell (i,j)
	bool wave_get(int i, int j, int pattern) const {
		return wave_get(i * _wave_width + j, pattern);
	}

	// Set the value of pattern in cell index.
	void wave_set(int index, int pattern, bool value);

	// Set the value of pattern in cell (i,j).
	void wave_set(int i, int j, int pattern, bool value) {
		wave_set(i * _wave_width + j, pattern, value);
	}

	// Return the index of the cell with lowest entropy different of 0.
	// If there is a contradiction in the wave, return -2.
	// If every cell is decided, return -1.
	int wave_get_min_entropy() const;

	void add_to_propagator(int y, int x, int pattern) {
		// All the direction are set to 0, since the pattern cannot be set in (y,x).
		CompatibilityEntry temp;
		_compatible.get(y, x, pattern) = temp;

		_propagating.push_back(PropagatingEntry(y, x, pattern));
	}

	constexpr int get_opposite_direction(int direction) {
		return 3 - direction;
	}

	void normalize(Vector<double> &v);
	Vector<double> get_plogp(const Vector<double> &distribution);
	double get_min_abs_half(const Vector<double> &v);

	void propagate();

	virtual void initialize();

	WaveFormCollapse();
	~WaveFormCollapse();

protected:
	static void _bind_methods();

	Array2D<int> _input;

	bool _periodic_output;

	//Wave
	int _wave_width;
	int _wave_height;
	int _wave_size;

private:
	RandomPCG _gen;

	// The number of distinct patterns.
	size_t _nb_patterns;

	// Transform the wave to a valid output (a 2d array of patterns that aren't in
	// contradiction). This function should be used only when all cell of the wave
	// are defined.
	Array2D<int> wave_to_output() const;

	// The patterns frequencies p given to wfc.
	Vector<double> _patterns_frequencies;

	// The precomputation of p * log(p).
	Vector<double> _plogp_patterns_frequencies;

	// The precomputation of min (p * log(p)) / 2.
	// This is used to define the maximum value of the noise.
	double _min_abs_half_plogp;

	Vector<double> _memoisation_plogp_sum; // The sum of p'(pattern)// log(p'(pattern)).
	Vector<double> _memoisation_sum; // The sum of p'(pattern).
	Vector<double> _memoisation_log_sum; // The log of sum.
	Vector<int> _memoisation_nb_patterns; // The number of patterns present
	Vector<double> _memoisation_entropy; // The entropy of the cell.

	// This value is set to true if there is a contradiction in the wave (all elements set to false in a cell).
	bool _is_impossible;

	// The actual wave. wave_data.get(index, pattern) is equal to false if the pattern can be placed in the cell index.
	Array2D<bool> _wave_data;

	//Propagator
	Vector<PropagatorStateEntry> _propagator_state;

	// All the tuples (y, x, pattern) that should be propagated.
	// The tuple should be propagated when wave.get(y, x, pattern) is set to false.
	Vector<PropagatingEntry> _propagating;

	// compatible.get(y, x, pattern)[direction] contains the number of patterns
	// present in the wave that can be placed in the cell next to (y,x) in the
	// opposite direction of direction without being in contradiction with pattern
	// placed in (y,x). If wave.get(y, x, pattern) is set to false, then
	// compatible.get(y, x, pattern) has every element negative or null
	Array3D<CompatibilityEntry> _compatible;

	void init_compatible();
};

VARIANT_ENUM_CAST(WaveFormCollapse::Symmetry);

#endif
