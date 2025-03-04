! RUN: %flang_fc1 -emit-hlfir %s -o - | FileCheck %s --check-prefixes=CHECK%if target=x86_64{{.*}} %{,CHECK-KIND10%}

! CHECK-LABEL: func @_QPspacing_test(
real*4 function spacing_test(x)
  real*4 :: x
  spacing_test = spacing(x)
! CHECK: %[[a1:.*]] = fir.load %{{.*}} : !fir.ref<f32>
! CHECK: %{{.*}} = fir.call @_FortranASpacing4(%[[a1]]) {{.*}}: (f32) -> f32
end function

! CHECK-KIND10-LABEL: func @_QPspacing_test2(
function spacing_test2(x)
  integer, parameter :: kind10 = merge(10, 4, selected_real_kind(p=18).eq.10)
  real(kind10) :: x, spacing_test2
  spacing_test2 = spacing(x)
! CHECK-KIND10: %[[a1:.*]] = fir.load %{{.*}} : !fir.ref<f80>
! CHECK-KIND10: %{{.*}} = fir.call @_FortranASpacing10(%[[a1]]) {{.*}}: (f80) -> f80
end function

! CHECK-LABEL: test_real2
subroutine test_real2(x, y)
  real(2) :: x, y
  y = spacing(x)
! CHECK: %[[CAST_ARG:.*]] = fir.convert %{{.*}} : (f16) -> f32
! CHECK: %[[RT_RES:.*]] = fir.call @_FortranASpacing2By4(%[[CAST_ARG]]){{.*}}: (f32) -> f32
! CHECK: fir.convert %[[RT_RES]] : (f32) -> f16
end subroutine

! CHECK-LABEL: test_real3
subroutine test_real3(x, y)
  real(3) :: x, y
  y = spacing(x)
! CHECK: %[[CAST_ARG:.*]] = fir.convert %{{.*}} : (bf16) -> f32
! CHECK: %[[RT_RES:.*]] = fir.call @_FortranASpacing3By4(%[[CAST_ARG]]){{.*}}: (f32) -> f32
! CHECK: fir.convert %[[RT_RES]] : (f32) -> bf16
end subroutine
