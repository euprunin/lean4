/-
Copyright (c) 2017 Microsoft Corporation. All rights reserved.
Released under Apache 2.0 license as described in the file LICENSE.
Authors: Gabriel Ebner, Sebastian Ullrich, Mac Malone
-/
import Lake.BuildMonad

namespace Lake
open System

def createParentDirs (path : FilePath) : BuildM PUnit := do
  if let some dir := path.parent then
    try IO.FS.createDirAll dir catch e => logError (toString e)

def proc (args : IO.Process.SpawnArgs) : BuildM PUnit := do
  let envStr := String.join <| args.env.toList.map fun (k, v) => s!"{k}={v.getD ""} "
  let cmdStr := " ".intercalate (args.cmd :: args.args.toList)
  logInfo <| "> " ++ envStr ++
    match args.cwd with
    | some cwd => s!"{cmdStr}    # in directory {cwd}"
    | none     => cmdStr
  let child ← IO.Process.spawn args
  let exitCode ← child.wait
  if exitCode != 0 then
    let msg := s!"external command {args.cmd} exited with status {exitCode}"
    logError msg -- log errors early
    throw <| IO.userError msg

def compileOlean (leanFile oleanFile : FilePath)
(oleanDirs : List FilePath := [])  (rootDir : FilePath := ".")
(leanArgs : Array String := #[]) (lean : FilePath := "lean")
: BuildM PUnit := do
  createParentDirs oleanFile
  proc {
    cmd := lean.toString
    args := leanArgs ++ #[
      "-R", rootDir.toString, "-o", oleanFile.toString, leanFile.toString
    ]
    env := #[("LEAN_PATH", SearchPath.toString oleanDirs)]
  }

def compileOleanAndC (leanFile oleanFile cFile : FilePath)
(oleanDirs : List FilePath := []) (rootDir : FilePath := ".")
(leanArgs : Array String := #[]) (lean : FilePath := "lean")
: BuildM PUnit := do
  createParentDirs cFile
  createParentDirs oleanFile
  proc {
    cmd := lean.toString
    args := leanArgs ++ #[
      "-R", rootDir.toString, "-o", oleanFile.toString, "-c",
      cFile.toString, leanFile.toString
    ]
    env := #[("LEAN_PATH", SearchPath.toString oleanDirs)]
  }

def compileO (oFile srcFile : FilePath)
(moreArgs : Array String := #[]) (compiler : FilePath := "cc") : BuildM PUnit := do
  createParentDirs oFile
  proc {
    cmd := compiler.toString
    args := #["-c", "-o", oFile.toString, srcFile.toString] ++ moreArgs
  }

def compileStaticLib (libFile : FilePath)
(oFiles : Array FilePath) (ar : FilePath := "ar") : BuildM PUnit := do
  createParentDirs libFile
  proc {
    cmd := ar.toString
    args := #["rcs", libFile.toString] ++ oFiles.map toString
  }

def compileBin (binFile : FilePath) (linkFiles : Array FilePath)
(linkArgs : Array String := #[]) (linker : FilePath := "cc") : BuildM PUnit := do
  createParentDirs binFile
  proc {
    cmd := linker.toString
    args := #["-o", binFile.toString] ++ linkFiles.map toString ++ linkArgs
  }
