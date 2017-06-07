; RUN: opt -load libjulia.so -CombineMulAdd -S %s | FileCheck %s

define double @fast_muladd1(double %a, double %b, double %c) {
top:
  %v1 = fmul fast double %a, %b
  %v2 = fadd double %v1, %c
  ;; Change the `{{contract| }}` to `contract` when we drop LLVM 4.0 support
; CHECK: %0 = call {{contract| }} double @llvm.fmuladd.f64(double %a, double %b, double %c)
  ret double %v2
; CHECK: ret double %0
}

define double @fast_mulsub1(double %a, double %b, double %c) {
top:
  %v1 = fmul double %a, %b
  %v2 = fsub fast double %v1, %c
; CHECK: %0 = fsub fast double -0.000000e+00, %c
; CHECK: %1 = call fast double @llvm.fmuladd.f64(double %a, double %b, double %0)
  ret double %v2
; CHECK: ret double %1
}

define double @fast_mulsub2(double %a, double %b, double %c) {
top:
  %v1 = fmul fast double %a, %b
  %v2 = fsub fast double %c, %v1
; CHECK: %0 = fsub fast double -0.000000e+00, %c
; CHECK: %1 = call fast double @llvm.fmuladd.f64(double %a, double %b, double %0)
; CHECK: %2 = fsub fast double -0.000000e+00, %1
  ret double %v2
; CHECK: ret double %2
}

define double @fast_mulsub3(double %a, double %b) {
top:
  %v1 = fmul fast double %a, %b
  %v2 = fsub fast double %v1, 1.000000e-01
; CHECK: %0 = call fast double @llvm.fmuladd.f64(double %a, double %b, double -1.000000e-01)
  ret double %v2
; CHECK: ret double %0
}

;; This is very unlikely going to be our input but just to make sure it doesn't crash
define double @fast_mulsub4() {
top:
  %v1 = fmul fast double 2.000000e-01, 3.000000e-01
  %v2 = fsub fast double %v1, 1.000000e-01
; CHECK: %0 = call fast double @llvm.fmuladd.f64(double 2.000000e-01, double 3.000000e-01, double -1.000000e-01)
  ret double %v2
; CHECK: ret double %0
}

define <2 x double> @fast_mulsub_vec1(<2 x double> %a, <2 x double> %b, <2 x double> %c) {
top:
  %v1 = fmul fast <2 x double> %a, %b
  %v2 = fsub fast <2 x double> %c, %v1
; CHECK: %0 = fsub fast <2 x double> <double -0.000000e+00, double -0.000000e+00>, %c
; CHECK: %1 = call fast <2 x double> @llvm.fmuladd.v2f64(<2 x double> %a, <2 x double> %b, <2 x double> %0)
; CHECK: %2 = fsub fast <2 x double> <double -0.000000e+00, double -0.000000e+00>, %1
  ret <2 x double> %v2
; CHECK: ret <2 x double> %2
}
