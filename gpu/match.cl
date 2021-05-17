
void get_words(__global char* pkt_buffer,
                 const size_t buffer_size,
                 const size_t pos,
                 float *word0x,
                 float *word0y,
                 float *word1x,
                 float *word1y,
                 float4 *tag0,
                 float4 *tag1)
{
    const size_t limit = min(7lu, buffer_size - pos);

    *word0x = (limit > 0) ? pkt_buffer[pos] : 0.0;
    *word0y = *word1x = (limit > 1) ? pkt_buffer[pos + 1] : 0.0;
    *word1y = (*tag0).s0 = (limit > 2) ? pkt_buffer[pos + 2] : 0.0;
    (*tag0).s1 = (*tag1).s0 = (limit > 3) ? pkt_buffer[pos + 3] : 0.0;
    (*tag0).s2 = (*tag1).s1 = (limit > 4) ? pkt_buffer[pos + 4] : 0.0;
    (*tag0).s3 = (*tag1).s2 = (limit > 5) ? pkt_buffer[pos + 5] : 0.0;
    (*tag1).s3 = (limit > 6) ? pkt_buffer[pos + 6] : 0.0;
}

__kernel void signature_match(__global char*    pkt_buffer,
                                const uint      buffer_size,
                              __global float2*  ans_buffer,
                              __global float4* table)

{
const size_t id = get_global_id(0);

    const size_t fst = id * 2;
    const size_t scd = fst + 1;

    float word0x = 0.0;
    float word0y = 0.0;
    float word1x = 0.0;
    float word1y = 0.0;
    float4 tag0 =  (float4)(0.0, 0.0, 0.0, 0.0);
    float4 tag1 =  (float4)(0.0, 0.0, 0.0, 0.0);

    get_words(pkt_buffer, buffer_size, fst, &word0x, &word0y, &word1x, &word1y, &tag0, &tag1);

    const size_t i = (size_t)word0x * 256 + (size_t)word0y;
    const size_t j = (size_t)word1x * 256 + (size_t)word1y;

    const float4 h0 = table[i];
    const float4 h1 = table[j];

    const float2 word0 = (float2)(word0x, word0y);
    const float2 word1 = (float2)(word1x, word1y);

    const float all_match0 = all(h0 == tag0);
    const float all_match1 = all(h1 == tag1);

    ans_buffer[fst] = word0 * all_match0;
    ans_buffer[scd] = word1 * all_match1;
}