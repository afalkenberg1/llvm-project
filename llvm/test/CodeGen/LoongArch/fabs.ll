; NOTE: Assertions have been autogenerated by utils/update_llc_test_checks.py
; RUN: llc --mtriple=loongarch32 --mattr=+32s,+f,-d < %s | FileCheck %s --check-prefix=LA32F
; RUN: llc --mtriple=loongarch32 --mattr=+32s,+d < %s | FileCheck %s --check-prefix=LA32D
; RUN: llc --mtriple=loongarch64 --mattr=+f,-d < %s | FileCheck %s --check-prefix=LA64F
; RUN: llc --mtriple=loongarch64 --mattr=+d < %s | FileCheck %s --check-prefix=LA64D

declare float @llvm.fabs.f32(float)
declare double @llvm.fabs.f64(double)

define float @fabs_f32(float %a) nounwind {
; LA32F-LABEL: fabs_f32:
; LA32F:       # %bb.0:
; LA32F-NEXT:    fabs.s $fa0, $fa0
; LA32F-NEXT:    ret
;
; LA32D-LABEL: fabs_f32:
; LA32D:       # %bb.0:
; LA32D-NEXT:    fabs.s $fa0, $fa0
; LA32D-NEXT:    ret
;
; LA64F-LABEL: fabs_f32:
; LA64F:       # %bb.0:
; LA64F-NEXT:    fabs.s $fa0, $fa0
; LA64F-NEXT:    ret
;
; LA64D-LABEL: fabs_f32:
; LA64D:       # %bb.0:
; LA64D-NEXT:    fabs.s $fa0, $fa0
; LA64D-NEXT:    ret
  %1 = call float @llvm.fabs.f32(float %a)
  ret float %1
}

define double @fabs_f64(double %a) nounwind {
; LA32F-LABEL: fabs_f64:
; LA32F:       # %bb.0:
; LA32F-NEXT:    bstrpick.w $a1, $a1, 30, 0
; LA32F-NEXT:    ret
;
; LA32D-LABEL: fabs_f64:
; LA32D:       # %bb.0:
; LA32D-NEXT:    fabs.d $fa0, $fa0
; LA32D-NEXT:    ret
;
; LA64F-LABEL: fabs_f64:
; LA64F:       # %bb.0:
; LA64F-NEXT:    bstrpick.d $a0, $a0, 62, 0
; LA64F-NEXT:    ret
;
; LA64D-LABEL: fabs_f64:
; LA64D:       # %bb.0:
; LA64D-NEXT:    fabs.d $fa0, $fa0
; LA64D-NEXT:    ret
  %1 = call double @llvm.fabs.f64(double %a)
  ret double %1
}
