#include "test_framework.h"

void run_image_tests();
void run_modes_tests();
void run_pd_all_modes_tests();
void run_encoder_lifecycle_tests();
void run_decoder_lifecycle_tests();

int main()
{
    run_image_tests();
    run_modes_tests();
    run_pd_all_modes_tests();
    run_encoder_lifecycle_tests();
    run_decoder_lifecycle_tests();

    const int checks = csstv_test::check_count();
    const int failures = csstv_test::failure_count();

    std::printf("\n==== %d checks, %d failed ====\n", checks, failures);

    return failures == 0 ? 0 : 1;
}
