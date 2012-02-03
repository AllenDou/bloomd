#include <string.h>
#include <math.h>
#include "sbf.h"

/**
 * Static declarations
 */
static int sbf_append_filter(bloom_sbf *sbf);

int sbf_from_filters(bloom_sbf_params *params, 
                     bloom_sbf_callback cb,
                     void *cb_in,
                     uint32_t num_filters,
                     bloom_bloomfilter *filters,
                     bloom_sbf *sbf)
{
    // Copy the params
    memcpy(&(sbf->params), params, sizeof(bloom_sbf_params));

    // Set the callback and its args
    sbf->callback = cb;
    sbf->callback_input = cb_in;

    // Copy the filters
    if (num_filters > 0) {
        sbf->num_filters = num_filters;
        sbf->filters = malloc(num_filters*sizeof(bloom_bloomfilter*));
        memcpy(sbf->filters, filters, num_filters*sizeof(bloom_bloomfilter*));
        sbf->dirty_filters = calloc(num_filters, sizeof(unsigned char));
    } else {
        sbf->num_filters = 0;
        sbf->filters = NULL;
        sbf->dirty_filters = NULL;

        int res = sbf_append_filter(sbf);
        if (res != 0) {
            return res;
        }
    }

    return 0;
}

/**
 * Adds a new key to the bloom filter.
 * @arg sbf The filter to add to
 * @arg key The key to add
 * @returns 1 if the key was added, 0 if present. Negative on failure.
 */
int sbf_add(bloom_sbf *sbf, char* key) {
    return 0;
}

/**
 * Checks the filter for a key
 * @arg sbf The filter to check
 * @arg key The key to check 
 * @returns 1 if present, 0 if not present, negative on error.
 */
int sbf_contains(bloom_sbf *sbf, char* key) {
    return 0;
}

/**
 * Returns the size of the bloom filter in item count
 */
uint64_t sbf_size(bloom_sbf *sbf) {
    return 0;
}

/**
 * Flushes the filter, and updates the metadata.
 * @return 0 on success, negative on failure.
 */
int sbf_flush(bloom_sbf *sbf) {
    return 0;
}

/**
 * Flushes and closes the filter. Does not close the underlying bitmap.
 * @return 0 on success, negative on failure.
 */
int sbf_close(bloom_sbf *sbf) {
    return 0;
}

/**
 * Returns the total capacity of the SBF currently.
 */
uint64_t sbf_total_capacity(bloom_sbf *sbf) {
    return 0;
}

/**
 * Returns the total bytes size of the SBF currently.
 */
uint64_t sbf_total_byte_size(bloom_sbf *sbf) {
    return 0;
}

/**
 * Appends a new filter to the SBF
 */
static int sbf_append_filter(bloom_sbf *sbf) {
    // Start with the initial configs
    uint64_t capacity = sbf->params.initial_capacity;
    double fp_prob = sbf->params.fp_probability;

    // Get the settings for the new filter
    capacity *= pow(sbf->params.scale_size, sbf->num_filters);
    fp_prob *= pow(sbf->params.probability_reduction, sbf->num_filters);

    // Compute the new parameters
    bloom_filter_params params = {0, 0, capacity, fp_prob};
    int res = bf_params_for_capacity(&params);
    if (res != 0) {
        return res;
    }

    // Allocate a new bitmap
    bloom_bitmap *map = calloc(1, sizeof(bloom_bitmap));

    // Try to use our call back if we have one
    if (sbf->callback) {
        res = sbf->callback(sbf->callback_input, params.bytes, map);
    } else {
        res = bitmap_from_file(-1, params.bytes, map);
    }
    if (res != 0) {
        free(map);
        return res;
    }

    // Create a new bloom filter
    bloom_bloomfilter *filter = calloc(1, sizeof(bloom_bloomfilter)); 
    res = bf_from_bitmap(map, params.k_num, 1, filter);
    if (res != 0) {
        free(filter);
        free(map);
        return res;
    }

    // Hold onto the old filters and dirty state
    bloom_bloomfilter *old_filters = sbf->filters;
    unsigned char *old_dirty = sbf->dirty_filters;

    // Increase the filter count, re-allocate the arrays
    sbf->num_filters++; 
    sbf->filters = malloc(sbf->num_filters*sizeof(bloom_bloomfilter*));
    sbf->dirty_filters = calloc(sbf->num_filters, sizeof(unsigned char));

    // Copy the old filters and release
    if (sbf->num_filters > 1) {
        memcpy(sbf->filters+1, old_filters, (sbf->num_filters-1)*sizeof(bloom_bloomfilter*));
        memcpy(sbf->dirty_filters+1, old_dirty, (sbf->num_filters-1)*sizeof(unsigned char));
        free(old_filters);
        free(old_dirty);
    }

    // Set the new filter, set dirty false
    sbf->filters = filter;
    sbf->dirty_filters = 0;

    return 0;
}

