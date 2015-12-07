#ifndef UU_MURMUR_H
#define UU_MURMUR_H

/*
 * The original implementation in C++ featured the following notice:
 *
 * MurmurHash3 was written by Austin Appleby, and is placed in the public
 * domain. The author hereby disclaims copyright to this source code.
 */

/**
 * rotate left a 32bit integer
 */
spdr_internal uint32_t rotl32(uint32_t const x, int8_t const r)
{
        return (x << r) | (x >> (32 - r));
}

/**
 * Block read - if your platform needs to do endian-swapping or can only
 * handle aligned reads, do the conversion here
 */
spdr_internal uint32_t private_murmur3_getblock(uint32_t const *p, int const i)
{
        return p[i];
}

/**
 * Murmur3 mix
 */
spdr_internal void murmur3_bmix32(uint32_t * non_aliasing h1,
                                  uint32_t * non_aliasing k1,
                                  uint32_t * non_aliasing c1,
                                  uint32_t * non_aliasing c2)
{
        *c1 = *c1 * 5 + 0x7b7d159c;
        *c2 = *c2 * 5 + 0x6bce6396;

        *k1 *= *c1;
        *k1 = rotl32(*k1, 11);
        *k1 *= *c2;

        *h1 = rotl32(*h1, 13);
        *h1 = *h1 * 5 + 0x52dce729;
        *h1 ^= *k1;
}

spdr_internal uint32_t
    murmurhash3_32(void const *key, int len, uint32_t const seed)
{
        const uint8_t *data = (const uint8_t *)key;
        const int nblocks = len / 4;

        uint32_t h1 = 0x971e137b ^ seed;

        uint32_t c1 = 0x95543787;
        uint32_t c2 = 0x2ad7eb25;

        /* body */
        {
                int i;
                const uint32_t *blocks = (const uint32_t *)(data + nblocks * 4);

                for (i = -nblocks; i; i++) {
                        uint32_t k1 = private_murmur3_getblock(blocks, i);

                        murmur3_bmix32(&h1, &k1, &c1, &c2);
                }
        }

        /* tail */
        {
                const uint8_t *tail = (const uint8_t *)(data + nblocks * 4);

                uint32_t k1 = 0;

                switch (len & 3) {
                case 3:
                        k1 ^= tail[2] << 16;
                case 2:
                        k1 ^= tail[1] << 8;
                case 1:
                        k1 ^= tail[0];
                        murmur3_bmix32(&h1, &k1, &c1, &c2);
                };
        }

        /* finalization */

        h1 ^= len;

        h1 *= 0x85ebca6b;
        h1 ^= h1 >> 13;
        h1 *= 0xc2b2ae35;
        h1 ^= h1 >> 16;

        h1 ^= seed;

        return h1;
}

#endif
