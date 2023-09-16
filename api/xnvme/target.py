"""
The :class:`.NPAPI` is an extension of the :class:`.CAPI` target.

TODO:

* Generate the actual command-prepper code

* Generate the function-body with shifts

* Add a nice header-description

* Add a nice src-description

Thus, the above files are what you should expect to see in the output-directory
"""
from yace.targets.capi.target import CAPI
from yace.emitters import Emitter
from pathlib import Path
from pprint import pprint
import logging as log


class NPAPI(CAPI):
    """
    Generate a C API just like :class:`.CAPI` and in addition to that, produce
    NVMe getter/setter functions.
    """

    NAME = "npapi"

    def emit_placeholder(self, model, hdr_path, src_path):
        """emit placeholder-code; for quick plumbing check."""

        with hdr_path.open("w") as hdrfile:
            hdrfile.write("int foo(int x, int y);\n")
        with src_path.open("w") as srcfile:
            srcfile.write("int foo(int x, int y) { return x + y; }\n")

    def emit_preppers(self, model, hdr_path, src_path):
        """Method creating preppers"""

        emitter = Emitter(Path(__file__).parent)

        # Populate 'preppers' with entities for command-construction-prepper,
        # and expand annotations with information for simplified
        # code-generation in the emitter-template
        ignore = ["opc", "opcode", "cid", "rsvd", "psdt", "fuse"]
        preppers = []
        for entity in (
            ent
            for ent in model.entities
            if ent.cls in ["struct"] and "nvme_nvm" in ent.sym
        ):
            opc = entity.ant.get("opc", None)
            if not opc:
                log.error("Missing ant.opc in: %s", entity.as_dict())
                return

            # This is an entity we want, add to preppers
            preppers.append(entity)

            # Annotate a list of parameters
            entity.ant["params"] = []

            for member in entity.members:

                # Annotate non-chosen members as such...
                if member.sym.startswith("cdw"):  # Generic fields => no prep
                    member.ant["chosen"] = False
                    continue
                if member.sym in ["mptr", "dptr"]:  # Payload-setup => no prep here
                    member.ant["chosen"] = False
                    continue
                if member.sym in ignore:  # Explicit ignore-list => no prep here
                    member.ant["chosen"] = False
                    continue
                if member.sym.startswith("rsvd"):  # Reserved fields => no prep
                    member.ant["chosen"] = False
                    continue

                member.ant["chosen"] = True

                # Annotate regular field
                if member.cls in ["field"]:  # Construct info for bit-operation
                    member.ant["typ"] = "uint%d_t" % member.typ.width
                    entity.ant["params"].append(member)
                    continue

                # Annotate bitfield
                sd = 0
                for bits in member.members:
                    sd += bits.width    # Increment shift-distance

                    if bits.sym in ignore:
                        bits.ant["chosen"] = False
                        continue
                    if "rsvd" in bits.sym:
                        bits.ant["chosen"] = False
                        continue

                    bits.ant["chosen"] = True

                    bits.ant["sd"] = sd - bits.width
                    bits.ant["mask"] = (1 << bits.width) - 1
                    bits.ant["typ"] = "bool" if bits.width == 1 else "uint8_t"
                    bits.ant["acc"] = f"{member.sym}.{bits.sym}"

                    entity.ant["params"].append(bits)

        with hdr_path.open("w") as hdrfile:
            hdrfile.write(
                emitter.render("file_prep.h", {"data": preppers, "meta": model.meta})
            )
        with src_path.open("w") as srcfile:
            srcfile.write(
                emitter.render("file_prep.c", {"data": preppers, "meta": model.meta})
            )

    def emit(self, model):
        """Emit code"""

        hdr_path = (self.output / f"lib{model.meta.prefix}_prep.h").resolve()
        src_path = (self.output / f"{model.meta.prefix}_prep.c").resolve()

        # self.emit_placeholder(model, hdr_path, src_path)
        self.emit_preppers(model, hdr_path, src_path)

        # Add the command-prepper code to the library-bundle and check-code
        # emitted by CAPI.
        self.headers.append(hdr_path)
        self.sources.append(src_path)
        super().emit(model)
